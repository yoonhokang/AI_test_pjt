# 프로젝트 내 가상/시험/시뮬레이션 구성 요소 리스트

**작성일:** 2026-02-02
**참조:** `@TestFw` Source Code Analysis

## 1. 개요
현재 `@TestFw` 프로젝트 코드에서 실제 하드웨어를 제어하지 않거나, 테스트 목적으로 Mocking(모의) 처리된 부분들을 식별하여 정리했습니다.

## 2. 시뮬레이션/가상 구현 리스트 (Simulated/Mock)

다음 모듈들은 현재 **실제 동작하지 않고 가상으로 동작**하도록 설정되어 있습니다.

| 모듈 | 파일 가이드 | 상태 | 상세 내용 |
| :--- | :--- | :--- | :--- |
| **Watchdog** | `Modules/Watchdog/Inc/watchdog_driver.h` | **Simulation** | `USE_HARDWARE_IWDG`가 `0`으로 설정됨.<br>실제 MCU 리셋을 수행하지 않고 로그(`[WDT] *Fed*`)만 출력함. |
| **Config Manager** | `Modules/Common/Src/config_manager.c` | **Mock** | Flash 메모리 읽기/쓰기가 구현되어 있지 않음.<br>`Config_Save()` 호출 시 실제 저장 없이 성공 메시지만 출력. |
| **TLS (mbedTLS)** | `mbedtls_stub.c` | **Stub** | 암호화 라이브러리의 핵심 기능이 빈 함수(Stub)로 구현됨.<br>빌드 에러 방지용이며, 실제 보안 통신은 되지 않음. |
| **OCPP Cert** | `Modules/OCPP/Src/ocpp_app.c` | **Dummy Data** | `mbedtls_root_ca` 변수에 테스트용 더미 인증서(GlobalSign Example)가 하드코딩되어 있음. |

## 3. 실제 하드웨어 동작 리스트 (Real Hardware)

다음 모듈들은 코드가 **실제 하드웨어와 연동**되도록 설정되어 있습니다. (Simulation Flag Off 확인됨)

| 모듈 | 파일 가이드 | 상태 | 상세 내용 |
| :--- | :--- | :--- | :--- |
| **Ethernet** | `w5500_driver.c` | **Real** | SPI 통신 코드가 정상 구현되어 W5500 칩과 통신함. |
| **Power Module** | `infy_power.h` | **Real** | `INFY_USE_SIMULATION`이 `0`임.<br>실제 CAN 메시지를 송수신함. |
| **Meter** | `meter_driver.h` | **Real** | `METER_SIMULATION_MODE`가 `0`임.<br>Modbus RTU 프로토콜 동작. |

---

## 4. 향후 조치 권장사항

1.  **Watchdog:** 하드웨어 검증 단계에서 `USE_HARDWARE_IWDG`를 `1`로 활성화해야 합니다.
2.  **Config Manager:** 양산 적용 시 STM32 내부 Flash 또는 외부 EEPROM을 이용한 실제 저장 로직 구현이 필요합니다.
3.  **TLS:** 보안이 필요한 경우 `mbedtls_stub.c`를 제거하고 실제 mbedTLS 라이브러리 파일(`library/*.c`)을 포함시켜야 합니다.
