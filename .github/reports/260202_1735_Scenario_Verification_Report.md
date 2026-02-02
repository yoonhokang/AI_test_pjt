# 시나리오 기반 펌웨어 동작 검증 보고서

**문서 번호:** 260202_1735_Scenario_Verification_Report  
**작성자:** Antigravity (Firmware Architect)  
**작성일:** 2026-02-02  
**대상 프로젝트:** TestFw

---

## 1. 개요 (Overview)
본 보고서는 사용자가 제시한 3가지 충전 시나리오(정상 충전/중단, 완충 종료, 예외 발생)에 대해 펌웨어의 코드 흐름을 추적하여 정상 동작 여부를 검증하고, 각 단계별 관여 기능을 설명합니다.

## 2. 시나리오별 검증 (Scenario Verification)

### 시나리오 1: 사용자 Start -> App Stop (중도 포기)
**검증 결과: 정상 동작 예상**

1.  **커넥터 연결:**
    -   `SECC Task`가 CP 전압(9V/6V) 변화를 감지하고, `secc_driver`를 통해 SECC 본체와 통신 개시.
    -   펌웨어는 `SECC_TxStatus`를 통해 충전기 상태(Ready)를 알림.
2.  **App 결제/시작:**
    -   `OCPP Task`에서 `RemoteStartTransaction` 메시지 수신 (`ocpp_app.c`).
    -   `StateMachine_RemoteStart()` 호출 -> 상태를 `STATE_PRECHARGE`로 변경.
3.  **충전 준비 (Pre-charge):**
    -   `STATE_PRECHARGE` 진입. `Power Module`을 Soft-Start 하여 EV 배터리 전압과 동기화.
    -   전압 매칭(Difference < 20V) 확인 후 `StateMachine`이 `STATE_CHARGING`으로 전환.
    -   `Relay_SetMain(true)` 실행 -> **DC 컨텍터 투입.**
4.  **충전 진행:**
    -   `App_ControlLoop` 내에서 `SECC`가 요청한 전압/전류(`secc_control` 구조체)를 `Infy_SetOutput`으로 파워 모듈에 전달.
    -   `OCPP_SendMeterValues`가 주기적으로 호출되어 충전량 정보를 서버(App)로 전송.
5.  **App 중단 (Stop):**
    -   `OCPP Task`에서 `RemoteStopTransaction` 메시지 수신.
    -   `StateMachine_RemoteStop()` 호출 -> 상태를 `STATE_CONNECTED`로 변경.
    -   **On Exit `STATE_CHARGING`:** `Relay_SetMain(false)` 실행 -> **DC 컨텍터 즉시 차단.**
    -   `OCPP_SendStopTransaction` 호출 -> 서버에 종료 및 최종 과금 정보 전송.

---

### 시나리오 2: 사용자 Start -> EV Full (완충 종료)
**검증 결과: 정상 동작 예상**

1.  **충전 진행 중 (상동):**
    -   EV의 SOC가 100%에 도달하면 BMS가 SECC에게 요청 전류를 0으로 보내거나, PowerDeliveryReq(Stop)을 전송.
    -   SECC는 CAN 메시지(`secc_control.allow_power`)를 0(False)으로 설정하여 펌웨어에 전달.
2.  **종료 감지:**
    -   `App_ControlLoop` 내 `StateMachine_Loop`에서 `secc_control.allow_power == 0` 조건을 감지.
    -   Auto-Stop 로직(`app_state.c`)이 트리거됨.
3.  **충전 종료 절차:**
    -   `Infy_SetOutput(0, 0, false)` -> 파워 모듈 출력 중단.
    -   `Relay_SetMain(false)` -> DC 컨텍터 차단.
    -   `StateMachine_SetState(STATE_CONNECTED)`로 전이.
    -   `OCPP_SendStopTransaction`으로 완충 사실 및 충전량 전송 -> App 팝업.

---

### 시나리오 3: 예외 처리 (Exception Handling)
**검증 결과: 적절한 보호 로직 확인됨**

#### 3.1 PLC 통신 중단
-   **동작:** SECC와의 CAN 통신이 1초 이상 두절되면 `SECC_IsConnected()`가 False를 반환.
-   **대응:** `app_state.c`의 `StateMachine_Loop`에서 `!SECC_IsConnected()` 조건 진입 시, 즉시 `Infy_SetOutput(0,0,false)` 및 `Relay_SetMain(false)`를 실행하여 시스템을 안전 상태(Safe State)로 강제 전환.

#### 3.2 계량 정확도 (Billing)
-   **동작:** `Meter_Driver`는 Modbus RTU를 통해 전용 DC 전력량계의 값을 디지털로 읽어옵니다. (AD 변환 오차 없음)
-   **대응:** `OCPP_SendMeterValues` 함수에서 `Meter_ReadEnergy()` 값을 그대로 모아서 전송하므로, 검정된 전력량계의 정밀도를 유지하며 과금 정보를 서버로 보낼 수 있습니다.

#### 3.3 안전 기능 (Safety Features)
-   **절연 불량 (ISO Fault):**
    -   `Safety_Check()`가 주기적으로 `IMD` 상태를 확인.
    -   절연 저항 < 100kΩ 시 `STATE_FAULT`로 전이 -> 충전 중단.
-   **융착 (Welding):**
    -   `app_state.c`의 `Welding Check` 로직 추가됨.
    -   `Relay_GetState() == OPEN`인데 `Meter_ReadVoltage() > 60V`가 2초 이상 지속되면 융착으로 판단하여 `STATE_FAULT` 발생.
-   **과열 (OverTemp):**
    -   Meter 또는 별도 센서의 온도가 85도를 넘으면 `Safety_Check`에서 차단.

## 3. 총평
본 펌웨어는 사용자가 제시한 시나리오를 정상적으로 수행할 수 있는 논리적 구조를 갖추고 있습니다. 특히 SECC(차량 통신)와 OCPP(서버 통신) 사이에서 Master/Slave 역할을 유연하게 수행하며, 비상 상황 시 하드웨어 보호를 최우선으로 하는 Failsafe 설계가 적용되어 있습니다.
