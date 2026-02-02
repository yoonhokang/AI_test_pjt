# 시스템 아키텍처 및 멀티태스킹 분석 보고서

**문서 번호:** 260202_1735_System_Architecture_Report  
**작성자:** Antigravity (Firmware Architect)  
**작성일:** 2026-02-02  
**대상 프로젝트:** TestFw

---

## 1. 개요 (Overview)
본 보고서는 `TestFw` 프로젝트의 펌웨어 구조를 분석하여 멀티태스킹 효율성, 모듈화 적절성, 코드 가독성을 평가한 결과를 기술합니다. 특히 통신 지연이 제어 로직에 미치는 영향을 중점적으로 검토하였습니다.

## 2. 멀티태스킹 구조 분석 (Multitasking Analysis)

### 2.1 태스크 구조
본 펌웨어는 **FreeRTOS (CMSIS-OS Wrapper)** 기반으로 설계되었으며, 크게 두 가지 핵심 태스크로 분리되어 있습니다.

| 태스크 명 | 함수 (`Function`) | 우선순위 | 주기 | 역할 |
| :--- | :--- | :--- | :--- | :--- |
| **App Task** | `App_ControlLoop` | **High** | 10ms | 하드웨어 제어, 안전 감시, SECC/Power CAN 통신 |
| **OCPP Task** | `App_OCPPLoop` | **Normal** | 10ms | 이더넷 통신, TLS 암호화, OCPP 메시지 처리 |

### 2.2 Blocking 이슈 검토
**[사용자 우려사항 1]** "OCPP 서버 응답 대기 중 파워 모듈 제어 명령 송신 지연?"
- **분석 결과:** **지연 발생하지 않음.**
- **근거:**
    - OCPP 통신은 별도의 `OCPP Task`에서 수행됩니다.
    - TCP 접속(`W5500_Connect_Poll`) 및 TLS 핸드쉐이크(`mbedtls_ssl_handshake`) 과정이 State Machine 형태로 구현되어 있어 Blocking을 최소화했으나, 설령 `mbedtls_ssl_read/write` 내부에서 네트워크 지연으로 인한 Blocking이 발생하더라도 이는 `OCPP Task`에만 영향을 줍니다.
    - **High Priority**인 `App Task`는 RTOS 스케줄러에 의해 선점(Preemption) 실행되므로, 10ms 주기의 제어 루프와 50ms 주기의 SECC 통신은 OCPP 상태와 무관하게 정해진 시간에 실행됩니다.

**[사용자 우려사항 2]** "DC 컨텍터 OPEN 시 통신 지연 영향?"
- **분석 결과:** **영향 없음.**
- **근거:**
    - `Safety_Check()` 함수는 `App_ControlLoop`의 최상단에서 매 루프마다 `Watchdog_Refresh` 직후 호출됩니다.
    - 비상 정지(E-Stop)나 치명적 오류 발생 시, 통신 태스크의 상태와 상관없이 즉시 `StateMachine_SetState(STATE_FAULT)`로 전이되며, 해당 함수 내에서 `Relay_SetMain(false)`가 하드웨어적으로 실행됩니다.

**[사용자 우려사항 3]** "CPU 유휴 상태 활용?"
- **분석 결과:** **적절함.**
- **근거:**
    - 각 태스크의 `while(1)` 루프 마지막에 `osDelay(10)`이 삽입되어 있습니다.
    - 이는 CPU 제어권을 Idle Task 또는 다른 낮은 우선순위 태스크로 넘겨주는 양보(Yield) 동작을 수행하므로, Busy-Waiting으로 인한 자원 낭비가 없습니다.

## 3. 모듈화 및 코드 품질 (Modularity & Quality)

### 3.1 모듈화 수준
프로젝트는 기능별로 디렉토리가 명확히 분리되어 있어 높은 수준의 응집도(Cohesion)를 보입니다.

- **App:** 어플리케이션 비즈니스 로직 (`app_state.c`, `app_main.c`)
- **Modules:** 하드웨어 추상화 계층 (HAL Driver 위에서 동작)
    - `SECC/`: 급속 충전 시퀀스 제어
    - `Power/`: 파워 모듈 CAN 프로토콜 (Infypower)
    - `Meter/`: 전력량계 Modbus 통신
    - `Relay/`: 고전압 릴레이 제어
    - `OCPP/`: 충전 서버 프로토콜

특히 `app_main.c`의 `App_RxDispatcher` 함수가 CAN ID를 기반으로 각 모듈(`SECC`, `Infy`, `IMD`)의 핸들러로 메시지를 라우팅하는 구조는 **확장성**이 우수합니다.

### 3.2 코드 가독성
- **네이밍:** `Module_Action` 형태의 일관된 접두사 사용 (예: `Relay_SetMain`, `SECC_TxStatus`)으로 가독성이 높습니다.
- **주석:** 주요 함수 헤더에 Doxygen 스타일 주석이 작성되어 있으며, 복잡한 로직(예: OCPP State Machine)에도 흐름을 설명하는 주석이 포함되어 있습니다.
- **복잡도:** `app_state.c`의 `StateMachine_Loop` 함수가 다소 길어질 수 있는 구조이나, 현재는 상태별로 `switch-case` 문이 잘 정리되어 있어 유지보수에 용이합니다.

## 4. 결론 (Conclusion)
분석 결과, `TestFw`는 **RTOS를 활용한 이중 태스크 구조**를 통해 통신 부하와 실시간 제어를 효과적으로 분리하고 있습니다.
사용자가 우려했던 '통신 지연에 의한 제어 마비' 현상은 구조적으로 방지되어 있으며, 모듈화 또한 유지보수하기 용이한 상태로 잘 되어 있다고 판단됩니다.
