# 구성품 인터페이스 및 펌웨어 연동 분석 보고서

**문서 번호:** 260202_1735_Component_Interface_Report  
**작성자:** Antigravity (Firmware Architect)  
**작성일:** 2026-02-02  
**대상 프로젝트:** TestFw

---

## 1. 개요 (Overview)
본 보고서는 `TestFw` 프로젝트가 제어하는 주요 하드웨어 구성품의 리스트를 식별하고, 각 구성품의 물리적/논리적 인터페이스가 펌웨어 내에 적절히 구현되었는지 검토한 결과를 기술합니다.

## 2. 구성품 리스트 및 인터페이스 (Component List & Interfaces)

| 구성품 (Module) | 역할 | 통신 인터페이스 | 연동 드라이버 파일 |
| :--- | :--- | :--- | :--- |
| **SECC** | 급속 충전 통신 제어기 | CAN (FDCAN1) @ 500kbps | `Modules/SECC/Src/secc_driver.c` |
| **Power Module** | AC/DC 전력 변환 (Infypower) | CAN (FDCAN2) @ 125kbps | `Modules/Power/Src/infy_power.c` |
| **IMD** | 절연 감시 장치 | CAN (FDCAN3) | `Modules/Safety/Src/imd_driver.c` |
| **Meter** | 전력량계 (DC Meter) | RS-485 (Modbus RTU) | `Modules/Meter/Src/meter_driver.c` |
| **Main Relay** | DC 출력 개폐 (+/- 동시 제어) | GPIO Output | `Modules/Relay/Src/relay_driver.c` |
| **OCPP Modem** | 충전 서버 통신 (W5500) | SPI (Ethernet) | `Modules/Ethernet/Src/w5500_driver.c` |

## 3. 인터페이스 구현 검토 (Implementation Verification)

### 3.1 SECC (Supply Equipment Communication Controller)
- **인터페이스 정의:**
    - TX: `SECC_TxStatus` (50ms 주기 전송, CP전압, 릴레이 상태 등을 SECC로 보고)
    - TR: `SECC_TxMeter` (200ms 주기 전송, 미터링 값 보고)
    - RX: `SECC_RxHandler` (충전 허용 여부, 목표 전압/전류 수신)
- **검토 결과:**
    - `App_ControlLoop`에서 50ms마다 `SECC_TxStatus`를 호출하여 주기적 통신 요구사항을 만족함.
    - Rx 데이터는 `secc_control` 구조체에 파싱되어 저장되며, `App StateMachine`이 이를 참조하여 충전 시퀀스를 제어함.
    - **적합함.**

### 3.2 Power Module (Infypower Rectifier)
- **인터페이스 정의:**
    - TX: `Infy_SetOutput` (목표 전압/전류 설정, Enable 비트 제어)
    - RX: `Infy_RxHandler` (모듈별 출력 전압/전류, Fault 상태 모니터링)
- **검토 결과:**
    - `Infy_SetOutput` 함수는 `hfdcan2`를 통해 브로드캐스트 ID로 제어 명령을 전송함.
    - 특히 최근 수정된 **Pre-charge Logic**에서, 릴레이 투입 전 전압 동기화를 위해 이 함수를 `Soft-Start` 목적으로 활용하는 로직이 적절히 통합됨.
    - **적합함.**

### 3.3 Safety & IMD (Insulation Monitoring)
- **인터페이스 정의:**
    - `Safety_Monitor`는 하드웨어 E-Stop (GPIO)과 IMD 상태를 취합.
    - IMD 데이터는 FDCAN3를 통해 수신 (`IMD_RxHandler`).
- **검토 결과:**
    - `Safety_Check()` 함수에서 `IMD_GetStatus()`를 호출하여 절연 저항(`insulation_resistance_kohm`)이 100kΩ 미만일 경우 즉시 Fault를 반환.
    - 하드웨어 E-Stop 핀(`GPIO_PIN_RESET` 시 Fault) 검사 로직이 포함됨.
    - **적합함.**

### 3.4 Meter (Billing Accuracy)
- **인터페이스 정의:**
    - Modbus RTU 프로토콜을 사용하며, 전압(`METER_REG_VOLTAGE`), 전류, 전력, 에너지를 순차적으로 조회.
- **검토 결과:**
    - `Meter_Process()` 함수가 State Machine 방식으로 구현되어 Blocking 없이 비동기적으로 데이터를 수집함.
    - 수집된 데이터(`meter_energy`)는 `OCPP_SendMeterValues`를 통해 서버로 전송됨.
    - **적합함.**

## 4. 결론 (Conclusion)
펌웨어 코드는 하드웨어 구성품들의 인터페이스 사양을 충실히 반영하고 있습니다.
- **CAN 버스 분리:** SECC(CAN1), Power(CAN2), IMD(CAN3)를 물리적으로 분리하여 트래픽 간섭을 최소화한 설계가 돋보입니다.
- **추상화 계층:** 각 하드웨어는 독립된 Driver 파일로 관리되어, 향후 하드웨어 변경 시 해당 Driver 파일만 수정하면 되는 구조입니다.
- **연동 상태:** 모든 주요 구성품의 초기화(`App_Init`), 주기적 데이터 교환(`App_ControlLoop`), 예외 처리(`Safety_Monitor`)가 유기적으로 연결되어 있습니다.
