# 펌웨어 인수인계 및 유지관리 가이드

**문서 번호:** 260202_1735_Handover_Guide  
**작성자:** Antigravity (Firmware Architect)  
**작성일:** 2026-02-02  
**대상 프로젝트:** TestFw

---

## 1. 프로젝트 정보 (Project Info)
- **프로젝트 명:** TestFw
- **타겟 MCU:** STM32G474RETx (Arm Cortex-M4)
- **개발 환경:** STM32CubeIDE (v1.14.0 이상 권장)
- **주요 미들웨어:** FreeRTOS, mbedTLS (OCPP 보안), W5500 Driver

## 2. 폴더 구조 (Structure)
유지보수를 위해 아래 핵심 디렉토리의 역할을 숙지해야 합니다.

- `Core/Src, Inc`: MCU 하위 레벨 설정 (Clock, DMA, GPIO 등). CubeMX 자동 생성 코드 포함.
- `App/`: 최상위 어플리케이션 로직.
    - `app_main.c`: 시스템 초기화 및 태스크 생성.
    - `app_state.c`: **수정 빈도 높음**. 충전 시퀀스 상태 머신.
- `Modules/`: 하드웨어 추상화 드라이버.
    - `Modules/SECC`: 충전 시퀀스 관련 메시지 변경 시 수정.
    - `Modules/OCPP`: 서버 프로토콜 변경 시 수정.
    - `Modules/Relay`: 하드웨어 변경(IO 맵핑 등) 시 수정.

## 3. 핵심 유지보수 포인트 (Maintenance Points)

### 3.1 Pre-charge 로직 수정 (최근 변경 사항)
이전 하드웨어(Pre-charge Relay 존재)와 달리, 현재 버전은 **Soft-Start 방식**을 사용합니다.
- **관련 파일:** `App/Src/app_state.c` (STATE_PRECHARGE 부분)
- **주의:** `Relay_SetPrecharge` 함수는 로깅만 수행하며 실제 GPIO를 제어하지 않음. 실수로 GPIO 제어 코드를 복구하지 말 것.

### 3.2 OCPP 서버 변경
서버 IP 및 포트는 `config_manager.h` 또는 `Config_Init()` 함수에서 설정됩니다.
- TLS/SSL 관련 인증서 교체가 필요한 경우 `mbedtls_ssl_conf_ca_chain` 관련 코드를 `ocpp_app.c`에서 확인하세요.

### 3.3 디버깅 (Debugging)
- **UART 로그:** USART2 (Virtual COM Port)를 통해 시스템 로그가 115200bps로 출력됩니다.
- **CLI:** 동일한 포트로 CLI 명령을 입력하여 강제로 릴레이를 제어하거나 상태를 조회할 수 있습니다. (`Modules/CLI` 참조)

## 4. 빌드 및 플래시 (Build & Flash)
1.  **Build:** Project -> Build Project (또는 Hammer 아이콘). '0 Errors, 0 Warnings' 확인.
2.  **Debug:** Run -> Debug Configurations -> STM32 Cortex-M C/C++ Application -> Debugger 탭 확인 (ST-LINK).

## 5. 자주 발생하는 이슈 (FAQ)
- **Q: 충전이 시작되지 않고 Timeout 발생**
    - A: SECC와의 CAN 통신이 정상인지 확인하세요. 로그에 `[SECC] Timeout`이 뜨는지 확인.
- **Q: OCPP 서버 접속 실패**
    - A: 이더넷 케이블 연결 및 공유기 DHCP 할당 여부를 확인하세요. `W5500_Init` 로그 확인.

---
**인계자 코멘트:** 코드는 모듈화가 잘 되어 있어 기능 추가가 용이합니다. 특히 안전 관련 로직(`Safety_Monitor`)은 수정 시 매우 신중을 기해주시기 바랍니다.
