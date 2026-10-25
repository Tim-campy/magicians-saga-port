# Próxima etapa: comparação com o Android original

Data: 2026-10-25. Extração e leitores nativos já têm validação estrutural/pixel a pixel. Ainda não houve execução do jogo original; esses testes não comprovam menus, animação, timing, saves ou dependências de rede.

## Ambiente conferido

- `tools/with-tools.cmd adb devices -l`: lista vazia na rechecagem atual.
- SDK/emulador não encontrados nos caminhos conhecidos; ADB local funciona.
- Host: aproximadamente 8 GB de RAM e 95 GB livres no volume C na consulta atual.
- Win32_ComputerSystem informa HypervisorPresent=true; Win32_OptionalFeature informa HypervisorPlatform InstallState=2 (desativado). Não confundir presença de um hypervisor do Windows com disponibilidade de WHPX para Android Emulator.
- Os indicadores de virtualização de Win32_Processor vieram false enquanto HypervisorPresent=true; isso não foi interpretado como prova de BIOS desativada.

A documentação oficial recomenda WHPX para aceleração no Windows e informa que sua ativação pode exigir reinicialização. Não foi alterado nenhum recurso do Windows nem iniciado reboot. [Android Emulator: aceleração](https://developer.android.com/studio/run/emulator-acceleration).

O APK só inclui ABIs ARM. Uma opção de emulador a avaliar é uma imagem Android 11 Google APIs com tradução ARM; isso é suporte documentado, não teste deste jogo. [Android Developers: executar apps ARM no emulador](https://android-developers.googleblog.com/2020/03/run-arm-apps-on-android-emulator.html). Nenhuma imagem de sistema foi baixada ainda.

## Escolha adiada pelo usuário

Atualização 2026-10-25: o usuário informou que não pode executar testes dinâmicos no momento e pediu continuidade estática. A escolha abaixo é necessária antes de executar a comparação, **não** antes de continuar pesquisa de código/formatos. Não tratar esta pendência como bloqueio geral do projeto.

Usar um dispositivo Android autorizado conectado por ADB, ou preparar um emulador local. Essa escolha afeta configuração do host e eventual reinicialização; o trabalho está pronto para comparação, mas o ambiente original precisa ser definido com o usuário. Não assumir autorização para interromper sua sessão remota por reboot.

## Primeiro cenário preparado

1. Confirmar dispositivo/serial, Android, tela e pacote `net.kairosoft.android.wizardquest` versão 1.4.0/code140. Registrar se já existem saves, sem limpá-los.
2. Observar boot pela activity `main.Main`, com entradas e condições registradas. Registrar prompts reais; não substituí-los por respostas presumidas.
3. Capturar primeiro estado estável e navegação inicial, relacionando screenshots aos PNGs/SEB/OPT e às referências de ResourceManager.
4. Comparar escala, offsets, inversões, alpha, fonte e gatilhos de áudio. Separar diferença de apresentação de diferença de regra.
5. Só então testar cenários controlados de pausa, retomada, rede e persistência; preservar o estado original e não alterar rede de uma sessão remota inadvertidamente.

Critério de saída desta rodada: referência de boot/primeiro formulário reproduzível, uma composição identificada comparável e lista de dependências/prompts observados. Não é ainda aceite de gameplay completo ou funcionamento offline.
