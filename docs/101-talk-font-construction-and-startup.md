# Inicialização de idioma e construção da fonte das falas

Data: 2026-10-25. Capítulo 4/M3. Evidência estática; nenhum callback Unity executado.

## Entrada de inicialização

IApplication.Start18197 (0x1b80c70), já identificado no ciclo Unity em22, chama Language.Init13986 em0x1b80d20. A chamada está no início do corpo, depois da preparação de metadados/tipo e antes dos ajustes de plataforma e Time.fixedDeltaTime. Não depende de um ramo de seleção de idioma dentro desse corpo. Language.Init contém as consultas de preferências e aplicação do pacote descritas em100.

Varredura de B/BL imediato nos segmentos executáveis do ELF encontrou somente esse site com destino0x1b146fc. O resultado foi conferido no disassembly do FDE; não cobre delegates, chamadas virtuais, reflexão ou execução. Não interpretar a contagem como prova de chamada única durante toda a vida do processo.

## Fonte criada para a fala

SubForm.CreateTalkForm36177 (0x1204bc4) aloca TextLayout e chama o construtor sem parâmetros19194 (0x1bbca54) em0x1204ca8. Depois configura tamanho196×60, velocidade1500, cor RGB(37,35,54), espaçamento4 e o texto da primeira página, conforme89/90. Nesse trecho não entrega uma fonte externa nem seleciona um arquivo TTF.

O construtor sem parâmetros consulta Language.Japanese13990 e encaminha `!Japanese` e true ao construtor em0x1bbcabc. Este:

- Cria Font para o campo fontBuf_+0xa0, chamando Font..ctor17759 (0x1b77698) em0x1bbcb98.
- Chama InitTextLayout19196 (0x1bbcbd4) em0x1bbcbb0.
- Escreve o primeiro argumento no campo **autoScale_+0x78** e o segundo em **localizeFontEnable_+0x85**, em0x1bbcbb4/0x1bbcbb8.

Portanto, no retorno normal desse construtor, autoScale_ é a negação do resultado de Japanese e localizeFontEnable_ é true. O primeiro argumento **não** é format_; esse outro campo fica em+0x6c. A configuração habilita caminhos posteriores, mas não determina sozinha a escala numérica finalmente aplicada.

InitTextLayout cria uma segunda Font por17759, em0x1bbcc50, e guarda no campo font_+0x18 em0x1bbcc54. Essa é a fonte principal do layout; fontBuf_ é um objeto distinto. InitTextLayout também lê uma escala da aplicação e guarda em+0x7c; a configuração efetiva dessa escala não foi fechada neste ciclo.

## Relação com a seleção de recursos

Font..ctor17759 chama Init17760. O caminho direto de Init chega a _init17761, que obtém GetDefaultGUIFont e GetGUIStyleId, escolhe face2, SetSize(0,true) e SetStyle(0). A seleção de recursos por idioma do relatório99 pertence, assim, à construção da fonte usada pela fábrica de falas; não é apenas uma função sem ligação conhecida com o formulário.

Init também possui caminho de despacho à UI, já identificado nos relatórios anteriores. Não há evidência nova de ativação de Font.useFontTexture_ nesse encadeamento. Ausência de store explícito nos trechos examinados não fecha todas as alterações posteriores do objeto. SetText, localização, ajuste de tamanho e desenho ainda precisam ser acompanhados antes de declarar um backend efetivo para toda a fala.

## Evidência e limites

- `build/research/language-init-callers.json`: candidatos B/BL imediato, com FDE e método associado.
- `build/research/talk-font-boot/`: Start, construtor sem parâmetros e InitTextLayout; `build/research/talk-layout-constructor/`: construtor com dois flags. TSVs/logs preservados; cobertura sintática integral.
- `build/research/form-data-preparation/M36177.asm`, `font-backend/M17759.asm`, `M17760.asm` e `font-init-body/M17761.asm`: cadeia conferida nas exportações existentes.
- Campos font_, fontBuf_, autoScale_ e localizeFontEnable_ conferidos nos metadados e tabela de offsets do ELF inventariado. Assembly usa base adicional0x100000; este relatório usa VAs do ELF.
- Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`. Sem leitura de saves/preferências do usuário, execução do APK ou alteração C++; último build permanece18/18.

Confiança **confirmed** para cadeia e argumentos; **unknown** para métricas e seleção após ajustes durante apresentação. Próximo: seguir SetText/ajuste de fonte e autoScale com esse objeto concreto, preservando idioma e escala como entradas explícitas enquanto seus valores não forem determinados.
