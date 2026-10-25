# RNG: cadeia de chamadas e seleção pendente

Data: 2026-10-25. Identidade do ELF e metadata em `07-il2cpp-code-map.md`. Ainda não há algoritmo portado, seed observada em execução ou teste diferencial contra Android.

## Primeira varredura

`tools/inspect_arm64_calls.py` examinou 16 métodos dentro dos FDEs e gravou `research/local/random-calls.json`: 81 transferências estáticas. Chamada presente não significa caminho executado; alvos virtuais continuam desconhecidos.

| Método | VA ELF | Observação de transferências diretas |
|---|---|---|
| kairo.unity.math.Random..cctor | 0x1128ce4 | JSystem.CurrentTimeMillis e JRandom..ctor |
| Random.Rand (índices 36631–36633) | 0x1128d9c / 0x1128e00 / 0x1128ea8 | JRandom.NextInt |
| Random.Hits / Hits1000 | 0x1128f34 / 0x1128f94 | Random.Rand |
| Random.Choose | 0x1128ff4 | Random.Hits |
| Random.Lot | 0x1129064 | Random.Rand |
| JRandom..ctor (sem seed) | 0x1b04b88 | CurrentTimeMillis e outro construtor JRandom |
| JRandom..ctor (índice 13182) | 0x1b04ba8 | System.Random..ctor, Random2018..ctor e Xorshift128..ctor |
| JRandom.SetSeed | 0x1b04e50 | Os mesmos três construtores |
| Random2018.Init | 0x1b050c8 | Type.GetTypeFromHandle e FieldInfo.SetValue |

Portanto **não se deve selecionar Xorshift128 ou System.Random por conveniência**: três implementações aparecem nos caminhos do wrapper. A condição de seleção e o estado precisam ser lidos antes de qualquer reconstrução. Reflexão em Random2018 também impede substituir simplesmente pela biblioteca padrão do compilador.

## Seleção e wrappers conferidos em assembly

Exportações locais em `build/research/rng`, seleção de 13 métodos em `build/research/rng.tsv`. Todos os endereços abaixo são VAs ELF (retirar 0x100000 dos endereços mostrados pelo projeto Ghidra).

- O construtor com seed **constrói as três implementações**, não apenas a selecionada. Ponteiros ficam em `this+0x10` (System.Random), `+0x18` (Random2018) e `+0x20` (Xorshift128). Uma flag byte do armazenamento estático, offset zero, força a seed a zero se não for zero; caso contrário passam os 32 bits baixos do argumento recebido. Metadata nomeia o candidato estático `fix_`, mas sua alteração no jogo ainda não foi rastreada. `SetSeed` recria os três objetos pelo mesmo padrão.
- Getter/Setter `Logic`, em `0x1b04af4`/`0x1b04b3c`, acessam inteiro no offset estático `+4`.
- `NextInt`, `0x1b04d9c`: valor Logic **2** seleciona `this+0x20`; **1**, `this+0x18`; demais valores, `this+0x10`. O objeto selecionado recebe chamada virtual pelo par em `classe+0x188`. O algoritmo final/override ainda precisa ser resolvido.
- `NextInt(bound)`, `0x1b04e20`: faz **uma chamada** a NextInt e usa `SDIV/MSUB` de 32 bits para obter resto com sinal. Não há algoritmo de rejeição visível nesse wrapper.
- `NextLong`, `0x1b04e3c`: faz **uma chamada** a NextInt e estende seu resultado de 32 para 64 bits com sinal; não gera dois valores.
- `Random.Rand()`, `0x1128d9c`, encaminha a NextInt da instância estática. `Rand(bound)`, `0x1128e00`, chama NextInt, nega condicionalmente resultado negativo e calcula resto com sinal. `Rand(min,max)`, `0x1128ea8`, calcula `max-min+1`, resto do NextInt por esse valor e soma min, usando instruções de 32 bits. Não substituir por `std::uniform_int_distribution`.
- `Hits(p)`, `0x1128f34`, compara `Rand(100) < p`; `Hits1000(p)`, `0x1128f94`, compara `Rand(1000) < p` (comparação signed LT). Não há clamp visível nesses wrappers.
- Inicializador `Random..cctor`, `0x1128ce4`, cria array de 256 elementos e uma instância JRandom com retorno de CurrentTimeMillis como argumento. Significado do array e momento efetivo de inicialização continuam desconhecidos.

Varredura dos BL/B imediatos do ELF não encontrou chamadas diretas ao setter Logic. Isso **não prova Logic fixo em zero**: inlining, atribuições diretas, reflexão e chamadas indiretas não são cobertos. Valor realmente usado, seeds e ordem de chamadas dependem de investigação dos chamadores e observação runtime.

Os casos limite de inteiros (overflow, mínimo assinado, divisor zero e intervalos inválidos) não foram executados; antes de portar, preservar semântica das instruções ARM64 em operações C++ sem comportamento indefinido. Não interpretar ausência de validação como autorização para substituir o comportamento por exceções ou correções.

## Vtable corroborada

A origem `0x138`, stride 16, corroborada em `13-animation-formats.md`, torna `classe+0x188` o slot 5. Leitura do metadata v31:

| Tipo / índice | vtableStart | Slot 5 codificado | Método resolvido |
|---|---:|---|---|
| System.Random / 247 | 2430 | 0x6000124b | 2341, System.Random.Next |
| Xorshift128 / 1728 | 20225 | 0x6000670f | 13191, Xorshift128.Next |
| Random2018 / 1729 | 20233 | 0x6000124b | 2341, System.Random.Next herdado |

Assim, Random2018 não usa um override próprio desse slot: sua inicialização altera o estado usado pelo método herdado. Isso não elimina a necessidade de recuperar sua inicialização/reflexão nem comprova qual opção o jogo seleciona.

Próximos alvos: alterações de Logic/fix_, algoritmos das implementações e um cenário de seed controlada comparado com o original.
