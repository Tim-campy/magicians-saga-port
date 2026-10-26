# Política estática de FPS e campos de taxa

Data: 2026-10-25. Complementa `09-native-callbacks.md` e `24-frame-conditions.md`. A configuração inicial identificada é **30**, mas isso não comprova frequência constante de gameplay nem 30 frames realmente apresentados por segundo.

## Método e identidade

Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços abaixo são VAs ELF; os arquivos `.asm` usam base Ghidra `+0x100000`.

Novas seleções `build/research/frame-policy.tsv` e `frame-policy-followup.tsv`, saídas nos diretórios homônimos, exportadas com `ExportBoundedInstructions.java`. Os 14 intervalos tiveram cobertura sintática integral; um é Config.ConvertExtension para investigação de recursos, não usado como evidência de FPS. Nenhum pseudocódigo ou execução do jogo foi usado.

A identidade dos dois blocos estáticos foi resolvida com relocação ELF, índice codificado e campo byvalTypeIndex da tabela v31:

| GOT ELF → slot metadata | Valor codificado | Índice nativo | Tipo metadata |
| --- | --- | ---: | --- |
| `0x27312d8→0x27c8788` | `0x20006f19` | 14220 | 4527, `cfg.MyConfig` |
| `0x272f5c0→0x27c5898` | `0x20004fef` | 10231 | 1778, `kairo.common.cfg.Config` |

Isso identifica a classe; não recupera automaticamente o nome de cada campo por offset.

## Origem do valor 30

`MyConfig..cctor`, método 38615, VA `0x1174274`, carrega **30** em w10 (`0x1174390`) e grava em seu bloco estático **+0x50** (`0x11743f4`). O bloco vem do GOT acima, via classe+0xb8. Nenhuma chamada interveniente modifica w10 nesse trecho.

`form.FormManager..ctor`, método 35755, VA `0x11d6644`, chama o construtor base e copia MyConfig.static+0x50 para **manager+0xb0** em `0x11d66c4–0x11d66d4`. `SetTargetFps`, método 19732, VA `0x1be4850`, consiste em `str w1,[x0,#0xb0]` e retorno. Assim, +0xb0 é o armazenamento do alvo padrão; o nome de campo original não precisa ser presumido para demonstrar a função.

`FormManagerBase.GetFps`, método 19729, lê **+0xb4**, em `0x1be4834`. Portanto o getter chamado GetFps não é um alias do armazenamento padrão usado por SetTargetFps. O cálculo do campo +0xb4 não foi seguido neste ciclo.

## Prioridade do getter GetTargetFps

`FormManagerBase.GetTargetFps` (19733, `0x1be17f8`) tem 60 bytes:

1. Se `IsFormChanging` retorna bit 0 ligado, retorna **120**.
2. Caso contrário, consulta GetTopForm. Se há formulário e o inteiro **form+0x4c != -1**, retorna esse inteiro.
3. Nos demais casos, retorna **manager+0xb0**.

`IsFormChanging` (19737, `0x1be4858`) testa manager+0xd8 contra null usando **JThread.op_Inequality** (13469, `0x1be7b48`); se a comparação for verdadeira retorna true, senão retorna `manager.byte+0xd1 !=0` (`0x1be48a0–0x1be48c4`). Não confundir presença/comparação do objeto com uma chamada IsAlive: essa não está presente no trecho.

O construtor `FormBase` (19610, `0x1bdc508`) inicializa **form+0x4c e +0x50 com -1**, via store de oito bytes em `0x1bdc5ac–0x1bdc5b0`. Os setters comprovam dois componentes distintos:

- `SetFrameRate` 19637 (`0x1bdd5f4`): primeiro argumento inteiro → +0x4c; segundo → +0x50, exceto que segundo=-1 reutiliza o primeiro; terceiro argumento `&1` → byte+0x54. Nenhuma validação de faixa aparece nesses 24 bytes.
- Sobrecarga 19638 (`0x1bdd60c`): apenas argumento booleano `&1` → byte+0x54, sem alterar os dois inteiros.

Não foram encontradas transferências diretas BL/B para essas duas sobrecargas nem para SetTargetFps na varredura dos segmentos executáveis descrita abaixo. Isso não prova ausência de uso: stores inline e chamadas indiretas continuam possíveis. Main.OnUpdate contém justamente stores diretos no campo +0xb0.

## Main.OnUpdate aplica a política ao Unity

No caminho normal de `main.Main.OnUpdate` (34538, `0x117cdc8`):

| Ordem | Evidência ELF | Alteração / chamada |
| --- | --- | --- |
| 1 | `0x117ce98`, `0x117ceb8–0x117cec4` | Obtém form.FormManager.GetInstance e copia MyConfig.static+0x50 para manager+0xb0. |
| 2 | `0x117cf64–0x117cf74` | Se Config.byte+0xc2 está ligado, grava **120** em +0xb0. |
| 3 | `0x117cf90–0x117cfb4` | Se Config.byte+0xc5 está ligado e Canvas.CheckKeyState(0x10) retorna bit 0 ligado, grava **400**. |
| 4 | `0x117cfcc–0x117cff4` | Se Config.byte+0xc4 está ligado e Canvas.CheckKeyState(0x400) retorna bit 0 ligado, grava **2**. Este store vem depois dos anteriores. |
| 5 | `0x117d000` | QualitySettings.set_vSyncCount(0). |
| 6 | `0x117d00c`, `0x117d034` | GetTargetFps e Application.set_targetFrameRate com o resultado preservado. |

Os inteiros 400/2 são possibilidades de código, não modos observados ou recomendações de taxa. As teclas são máscaras conservadas como números, sem lhes atribuir teclas físicas por suposição. A prioridade do getter ainda permite que transição/formulário substituam o campo padrão alterado acima.

`form.FormManager.Updated` (35758, `0x11d6a14`) também consulta MyConfig.static+0x50 e Config.byte+0xc5. Com esse seletor ligado e CheckKeyState(0x10) **ou** CheckKeyState(0x20) verdadeiro, escolhe **120** (`0x11d6aec–0x11d6b40`). Compara a escolha a GetTargetFps e só quando diferentes grava manager+0xb0 (`0x11d6b44–0x11d6b58`). Não é idêntico à política do Main.OnUpdate; ordem efetiva dentro do loop e setters de configurações permanecem trabalho posterior.

## Valores iniciais de Config relevantes

`Config..cctor` (13622, `0x1bed0a0`) escreve valores explícitos no bloco estático:

| Offset | Valor inicial observado | Evidência ELF |
| --- | ---: | --- |
| +0x30 | 1 | w10=1 em `0x1bed2e0`, store byte `0x1bed318` |
| +0x63 | 0 | store de 32 bits zero em +0x60, `0x1bed35c` |
| +0xc2 | 0 | `0x1bed3ac` |
| +0xc4 / +0xc5 | 1 / 1 | w11=0x01010101 em `0x1bed2e8`, store não alinhado de quatro bytes em +0xc3, `0x1bed3b0` |

Esses são valores após o inicializador, não constantes imutáveis. Em particular, a espera manual de OnGUI descrita em `24` é habilitada pelo byte +0x63, inicialmente zero. Falta inventariar seus escritores antes de declarar se a espera ocorre no Android.

O mesmo inicializador copia 16 bytes do ELF **0x625fb0** para Config.static+0 em `0x1bed2dc–0x1bed2f4`. Os quatro inteiros little-endian são `(2,0,100,30)`; logo o seletor inicial +0 é 2. Essa observação também subsidia a conversão de extensões documentada na pesquisa de recursos, sem garantir que outro código não o altere.

## Frequência de apresentação não equivale a quantidade de updates

`FormManagerBase._execute` (19675, `0x1be01d8`) usa tanto form+0x50 quanto GetTargetFps. Um trecho em `0x1be0df4–0x1be0e58` calcula quantidade de passagens com divisões inteiras e resto acumulado em manager+0x98; se form+0x50=-1, usa inicialmente uma passagem. Há um loop que pode enviar o evento de código 1 mais de uma vez (`0x1be0e64–0x1be0f30`), sujeito a estado e troca do formulário. Outro ramo, habilitado por form.byte+0x54, considera tempo decorrido e limita-o usando `2000 / GetTargetFps` (`0x1be0f64–0x1be0ff0`).

Esse trecho ainda não é uma especificação completa: falta resolver o significado do evento, todos os estados e a correção em registrador s8. Ele já basta para **não implementar um update de gameplay por frame apresentado** com base somente no valor 30. Não foi criado código de simulação aproximado.

## Varredura de referências e cobertura

Varredura local dos words ARM64 alinhados nos segmentos PT_LOAD executáveis para BL/B imediato aos quatro destinos SetFrameRate(2), SetTargetFps e GetTargetFps; resultados em `build/research/frame-policy/direct-references.json`. Foram sete referências diretas, todas a GetTargetFps: Main.OnUpdate, FormManager.Updated, IApplication.OnGUI e quatro sites em FormManagerBase._execute. Os sete sites foram confrontados com assembly delimitado por FDE, e não tratados como grafo de execução.

Limites: não cobre stores inline equivalentes aos setters, chamadas por delegate/vtable, reflexão, ponteiros nem alcance de cada caminho. Não infere ausência de manipulações da taxa a partir de ausência de BL. Os relatórios coverage.tsv validam extensão e bytes dos exports, não o significado de todos os blocos.

Próximo passo estático delimitado: escritores de MyConfig+0x50/Config+0x63 e caminho de eventos FormManagerBase._execute, preservando separação entre frequência solicitada ao Unity, taxa solicitada pelo formulário e quantidade de eventos lógicos. Frequência observada e fidelidade de timing continuam reservadas a testes futuros.

Continuação: [28-form-events.md](28-form-events.md) resolve evento1 como Update, recupera a fórmula de passagens e a constante0,1f. [29-seb-animation-timing.md](29-seb-animation-timing.md) separa avanço manual/automático SEB e contador por entidade. As lacunas anteriores ficam registradas como histórico, não como bloqueio para investigação.
