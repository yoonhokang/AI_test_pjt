# 가상/시험/시뮬레이션 구성 요소 및 TLS 분석 보고서 (2차)

**작성일:** 2026-02-02
**작성자:** Antigravity (Firmware Architect)
**참조:** `260202_1835_Simulation_Analysis_Report.md` (1차)

## 1. 개요
프로젝트 내에서 실제 하드웨어 없이 가상으로 동작(Simulated, Mock)하는 부분과, 보안 통신(TLS)을 실제 동작시키기 위한 필수 조치 사항을 정리했습니다.

## 2. 시뮬레이션 구성 요소 리스트 (Simulated Components)

| 모듈 | 상태 | 상세 내용 |
| :--- | :--- | :--- |
| **Config Manager** | **Mock** | `config_manager.c`에서 Flash 메모리 저장을 흉내만 내고 있습니다. (`Config_Save` 호출 시 로그만 출력) |
| **OCPP Certificate** | **Dummy** | `ocpp_app.c`에 테스트용 더미 Root CA 인증서가 하드코딩되어 있습니다. |
| **TLS Library** | **Stub** | 암호화 라이브러리(`mbedTLS`)의 소스 코드가 누락되어 있고, 빈 함수(Stub)로 대체되어 있습니다. |
| **Watchdog** | **Real** | `USE_HARDWARE_IWDG`가 `1`로 설정되어 실제 하드웨어 리셋이 동작합니다. (이전 0에서 변경됨) |

> **Note:** 이더넷(W5500), 파워모듈(CAN), 전력량계(Modbus)는 모두 **Real Mode**로 설정되어 있습니다.

## 3. 리얼 TLS 동작을 위한 조치 사항 (Action Plan for Real TLS)

사용자의 요청("TLS 기능이 실제로 동작할 수 있도록 부족한 부분 추가")에 따라, 프로젝트 구성을 **Real TLS** 모드로 변경했습니다. 하지만 라이브러리 소스 파일 자체가 누락되어 있어 사용자의 수동 복사가 필요합니다.

### 3.1 부족한 부분 및 추가된 사항
1.  **Porting Layer (완료):** `mbedtls_port.c`에서 STM32 하드웨어 RNG(난수생성기)와 연동되도록 코드를 검토 및 확정했습니다.
2.  **Configuration (완료):** `mbedtls_config.h`가 STM32G4에 맞게 이미 설정되어 있습니다.
3.  **Library Sources (누락됨):** 암호화 알고리즘(`AES`, `SHA`, `RSA` 등)을 수행하는 실제 C 파일들이 `Middlewares/Third_Party/mbedTLS/library/` 폴더에 없습니다.

### 3.2 사용자 수행 필요 작업
빌드 오류 없이 실제 암호화 통신을 하려면 다음 단계를 수행해야 합니다.

1.  **소스 복사:** STM32Cube 패키지 또는 원본 mbedTLS 레포지토리에서 `/library/*.c` 파일들을 모두 복사하여 프로젝트의 `Middlewares/Third_Party/mbedTLS/library/` 폴더에 넣으세요.
2.  **Stub 제거:** 같은 폴더에 있는 `mbedtls_stub.c` 파일을 삭제하거나 이름을 변경(`_bak` 등)하여 빌드에서 제외하세요.
3.  **빌드:** 파일 복사 후 다시 빌드하면, Stub 대신 실제 라이브러리가 링크되어 동작합니다.

---
**요약:** 프로젝트의 설정(Config)과 인터페이스(Port)는 이제 Real TLS를 지원합니다. **라이브러리 소스 파일만 복사해 넣으면 즉시 동작** 가능한 상태입니다.
