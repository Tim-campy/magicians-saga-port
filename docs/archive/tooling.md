# Ferramentas e instalação local

Instalação local concluída no ciclo atual: apktool 2.12.1 (digest oficial verificado), ADB 1.0.41 / Platform Tools 37.0.1-15733141. Versões e hashes reais em `.tools/installed-tools.json`; comandos `tools\with-tools.cmd apktool --version` e `tools\with-tools.cmd adb version` passaram. JADX existente preservado. Nenhuma política Windows ou PATH global alterado. Instalar ADB não demonstra disponibilidade de dispositivo/emulador.

Ghidra ARM64 validado diretamente: `C:/Ghidra/support/analyzeHeadless.bat` importou libmain ELF com linguagem AARCH64 e concluiu análise/decompilação com 768 MiB e uma CPU. O limite do bridge REA permanece, mas não bloqueia essa rota. Scripts próprios em `tools/ghidra`; comandos e resultados em `06-native-bootstrap.md`.

## Estado atual: CLI funcional no agente

O wrapper agora define `REA_JADX_HEAP_MIB=768` quando não há override. Executar consultas sequencialmente neste host: quatro JVMs simultâneas com heap padrão provocaram erro de memória virtual Windows 1455 e falha de cleanup REA. Consultas repetidas com heap reduzido e sem paralelismo passaram. Detalhes e evidências em `04-android-startup.md`.

Após a mudança explícita das permissões do ambiente, a CLI REA executou com sucesso a inspeção de `main.Main` e a decompilação de `onCreate`, sem erro Win32 5. Criado `tools/rea-local.cmd`: define `REA_JADX_MCP_JAR` somente no processo filho e chama a CLI existente. Não exige alterações globais nem configuração do MCP já iniciado.

Comando validado na raiz do projeto:

```powershell
tools\rea-local.cmd inspect-android-method "Magician's+Saga_1.4.0_APKPure.apk" main.Main onCreate --overload-index 0 --format json
```

Evidências locais: `research/local/main-class.json` e `research/local/main-oncreate.json`. As notas de acesso negado abaixo são históricas.

Atualização: o usuário forneceu execução bem-sucedida de `rea.cmd inspect-android-package` com JADX 0.7.1: estado LOADED, 12.499 classes, manifesto completo. A instalação e integração CLI no terminal do usuário estão validadas. Os bloqueios relatados abaixo se referem ao ambiente do agente ou ao histórico anterior; veja `research-log.md`.

## Estado atual (substitui os bloqueios históricos abaixo)

Em 2026-10-25, REA/JADX pela CLI local, Ghidra headless ARM64 direto, apktool 2.12.1 e ADB 1.0.41 foram executados com sucesso. `tools/rea-local.cmd` limita heap JADX a 768 MiB por padrão. ADB lista zero dispositivos; não há emulador nos caminhos conhecidos. Java e Python existentes foram preservados. Nenhum PATH global ou política PowerShell foi alterado.

UnityPy 1.25.4 funciona em `.tools/unitypy-libs`; pyelftools 0.32 em `.tools/python-libs`. `uv` está disponível em `C:/Users/l_abn/.local/bin/uv.exe`; extração e verificação por `uv run tools/extract_assets.py` passaram. FFmpeg/ffprobe estão disponíveis em `C:/ffmpeg/bin`.

CMake e compiladores C++ não estão no PATH normal, mas o Visual Studio Community em `C:/Program Files/Microsoft Visual Studio/18/Community` contém MSVC 19.51.36252.0, Windows SDK 10.0.26100.0, CMake 4.3.1-msvc1 e Ninja 1.13.2. Build C++17 x64, CTest e leitor nativo passaram via `tools/build-desktop.cmd`, que configura apenas o processo filho. Nenhuma instalação de compilador foi necessária. Linux ainda não foi executado.

Os relatos de erro de rede, self-test e instalação pendente a seguir são **históricos**, não o estado atual. Resultados de análises em `04` a `13`; registro das instalações em `.tools/installed-tools.json`.

## Revalidação histórica após instalação manual

O JAR `.tools/jadx/jadx-headless-mcp-0.7.1-all.jar` está presente (42.619.858 bytes), com SHA-256 `6e5eacf500b64292bfb73c49797c1958f6ee44646e43e868039ae7feb573ff75`, igual ao checksum fornecido. `java -jar .tools/jadx/jadx-headless-mcp-0.7.1-all.jar --help` terminou com código 0.

A integração ainda não passou: o servidor MCP existente não recebeu `REA_JADX_MCP_JAR`; a CLI com essa variável definida falhou no self-test nativo REA com `Open Windows path component failed [C:\Users\l_abn] (Win32 5): Acesso negado`. Não alterar permissões ou contornar o self-test. Nenhuma análise JADX do APK foi concluída nesse teste. As notas abaixo registram o estado anterior à instalação manual.

Verificação em 2026-10-25:

| Ferramenta | Estado |
|---|---|
| Python | 3.12 disponível; inventariador sem pacotes externos |
| Java/Javac | JDK Microsoft 21.0.12.1 disponível |
| REA | MCP 6.5.0 conectado; launcher `rea.cmd` disponível |
| Ghidra | 12.1.4 em `C:\Ghidra`, confirmado pelo diagnóstico REA |
| JADX MCP | JAR não configurado; chamada REA retornou `capability_unavailable` |
| apktool / ADB | Não encontrados no PATH nem disponíveis nos providers REA |
| CMake / binutils | Não encontrados no PATH; não necessários para inventário Python |

Skill REA local versão 37; servidor anuncia expectativa 42. As chamadas seguiram os schemas efetivamente anunciados. A pasta da skill contém SKILL.md e referências; nenhum agente especializado foi encontrado ali. Foram usados subagentes limitados para o inventariador e o setup, conforme AGENTS.md.

Ghidra instalado não significa que o bridge REA Windows aceite este alvo: o perfil anunciado admite PE x86/x64 e casos raw específicos, não ARM ELF. Não houve tentativa de descompilar `.so` com um perfil incompatível.

## Instalação autorizada, ainda pendente

`tools/setup-tools.ps1` prepara apenas `.tools/`: JADX headless MCP 0.7.1, apktool 2.12.1 e Android Platform Tools (ADB). Não modifica PATH global nem política PowerShell. JDK/Ghidra existentes são preservados; toolchain C++ fica para depois da decisão arquitetural.

Fontes oficiais: [JADX MCP 0.7.1](https://github.com/1013503897/jadx-headless-mcp/releases/tag/v0.7.1), [apktool 2.12.1](https://github.com/iBotPeaches/Apktool/releases/tag/v2.12.1), [Android Platform Tools](https://developer.android.com/tools/releases/platform-tools).

JADX é verificado pelo checksum publicado, apktool pelo digest da release. O pacote Google usa URL móvel: seu hash e versão serão registrados, sem alegar comparação com checksum externo. ADB instalado não fornece automaticamente emulador ou dispositivo.

Em terminal com rede e execução de scripts permitida:

```powershell
powershell -NoProfile -File tools/setup-tools.ps1
tools\with-tools.cmd apktool --version
tools\with-tools.cmd adb version
tools\with-tools.cmd rea.cmd inspect-android-package --help
```

O wrapper exporta `REA_JADX_MCP_JAR` e PATH somente para o processo filho. Isso não reconfigura o MCP já em execução; usar a CLI com o wrapper ou configurar o servidor em uma etapa autorizada posterior. Nenhuma configuração global foi alterada.

A conexão de teste a `https://api.github.com` falhou (`curl` código 7). A tentativa de executar `powershell -NoProfile -File tools/setup-tools.ps1` foi recusada pela política de execução PowerShell, antes de qualquer instalação. `rea.ps1` também foi recusado; `rea.cmd` evita depender desse launcher PowerShell sem alterar a política. Não há instalação concluída nem teste de análise JADX bem-sucedido nesta sessão. Após disponibilidade de rede e execução permitida, o instalador fará validação apktool/ADB e registrará `.tools/installed-tools.json`; análise real JADX continuará necessária.
