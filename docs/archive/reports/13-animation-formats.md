# M0 — Inventário de animações `.seb` e índices `.inf`

**Atualização:** o ciclo seguinte produziu parser **estrutural** dos modos SEB 0/1, com consumo exato e round trip dos 303 arquivos. Schema e testes estão na seção final. As observações anteriores preservam a sequência de validação; a semântica completa de animação/renderização continua pendente.

Data: 2026-10-25. Inventário dos arquivos locais e do catálogo IL2CPP/mapa nativo, seguido de uma exportação Ghidra delimitada dos dois leitores candidatos. Sem execução do jogo ou implementação de renderer. A extração de recursos cifrados foi explicitamente autorizada pelo usuário. O formato de animação ainda não está integralmente recuperado.

## Entrada e validação

APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`. Entrada: `data/extracted/manifest.json`, schema 2, SHA-256 `7a30b5c9df677d9e328e6ab10ffb8bc080940f8e7f46e1f5f225c9ea36ae13f4`. Seus 1.963 membros foram produzidos por `tools/extract_assets.py`; os arquivos proprietários permanecem locais, fora do código distribuído.

Foram selecionados pelo **nome original no manifesto**, não pelo nome físico com prefixo numérico, todos os membros `.seb`/`.inf`. Tamanho e SHA-256 dos **366 arquivos** foram comparados com o manifesto, sem divergência:

| Extensão | Arquivos | Total de bytes |
| --- | ---: | ---: |
| `.seb` | 303 | 56.576 |
| `.inf` | 63 | 29.448 |

As VAs abaixo são endereços ELF ARM64 do mapa validado em [07](07-il2cpp-code-map.md), não offsets de arquivo nem endereços runtime.

## `.inf`: existem índices e configurações diferentes

Todos os 63 arquivos decodificam como UTF-8. Os nomes são 24 `img.inf`, 24 `seb.inf`, dois `English.lproj/img.inf`, um `zh-CN/img.inf` e 12 arquivos `optimize_*.inf`.

Os índices de imagem/SEB contêm linhas com identificador decimal, TAB e nome de recurso; alguns nomes têm sufixo de opção `,bin`. Não assumir que isso significa um arquivo adicional de extensão `.bin`. Há três linhas sem identificador explícito: `bg.seb` em lineup_layout e `talk.seb`/`bustup.seb` em resident. Outra linha começa com espaço antes de `0`. Um parser rígido que exige `isdigit()` antes de remover espaços rejeitaria conteúdo legítimo observado.

Após apenas remover espaços externos e separar um TAB, há **1.298 linhas não vazias** nos índices de imagem/SEB. Um teste literal de nome relativo ao diretório do índice e mesmo contêiner encontrou **103 nomes sem correspondência exata**: 80 `.png`, 21 `.gif`, dois `.seb`. Isso é um resultado de busca literal, **não** uma prova de conteúdo ausente ou download externo. Exemplos:

- `lineup_layout/img.inf` referencia `bg.gif`, mas o contêiner tem `bg.png`.
- `room/img.inf` referencia `bg_long.png`; o contêiner tem `bg0.png` e `bg1.png`, e também `bg_long.seb`. A relação de composição ainda não foi resolvida.
- `room/seb.inf` referencia `equip_frame01.seb` e `equip_frame02.seb`, sem membros de mesmo nome nesse contêiner.
- Índices localizados precisam de regras de fallback confirmadas no leitor; não presumir que todo nome é relativo exclusivamente à pasta de idioma.

`optimize_30x30.inf` tem pares chave/valor com `=`, incluindo `size`, `merge`, `compare`, `external_merge`, `external_compare`, `level`, `color_range`, `width` e `output`. São configurações distintas dos índices. Não foi estabelecido se são consumidas em runtime ou restos do pipeline de produção.

## `.seb`: observações de bytes, sem schema presumido

O menor tamanho observado é **28 bytes**; o maior é **2.672 bytes**. Ler os primeiros oito bytes como quatro inteiros unsigned big-endian de 16 bits dá padrões recorrentes, mas ainda não define o significado dos campos:

| Membro local | Bytes | Quatro valores BE16 iniciais |
| --- | ---: | --- |
| `members/01_load/0004_copy.seb` | 28 | 1, 1, 1, 1 |
| `members/00_hat/0149_hat_com.seb` | 48 | 1, 2, 2, 2 |
| `members/02_weapon/0093_weapon_com.seb` | 88 | 1, 4, 4, 4 |
| `members/30_chara/0088_ch_bow_attack_right.seb` | 1.024 | 5, 15, 10, 15 |
| `members/11_com/0121_fukidasi.seb` | 2.672 | 2, 24, 65, 100 |
| `members/20_select/0079_map.seb` | 400 | 14, 1, 4, 10 |
| `members/27_enemy/0088_k.seb` | 1.024 | 65280, 256, 7680, 15104 |

A distribuição do primeiro BE16 é: **1 em 256 arquivos; 5 em 29; 2 em 12; 3 em quatro; 14 em um; 0xff00 em um**. Portanto não tratá-lo como uma versão fixa. O arquivo `k.seb` começa `ff0001001e003b00`; pode ter um layout distinto, mas essa possibilidade ainda não foi comprovada pelo leitor.

Uma hipótese puramente numérica, `tamanho = 8 + 20 × quarto_BE16`, funciona em **154/303** arquivos e falha nos demais. Foi registrada para refutar a adoção prematura de um registro uniforme, não como parser. Não há confirmação de contagens de frames, camadas, índices de imagem, duração, ordem de composição ou endian global. Valores que lembram coordenadas negativas não devem ser tipados como signed sem observar o consumidor.

Localizadores de identidade para repetir o experimento:

| Arquivo | SHA-256 |
| --- | --- |
| `01_load/0004_copy.seb` | `64562b959e74ed2221a67c3247f750cc4cdb9abce47ad69f2db7659c3c26eebd` |
| `00_hat/0149_hat_com.seb` | `c5957003f9d26d71376edcfbb10f76437ea174312f37a4c3536d8bee149bef8c` |
| `02_weapon/0093_weapon_com.seb` | `fb899fddc607144ebbb18b80ead12aa589185676a9d585f034f0a84e147923c1` |
| `27_enemy/0088_k.seb` | `146ea07ed91ba7e29d98c11eee86cb7361bbf1fd2590fe327479572a054cc95f` |

Os caminhos da tabela são relativos a `data/extracted/members/`. O manifesto fornece as identidades dos outros arquivos e o contêiner de origem.

## Leitores candidatos localizados

Todos os métodos abaixo pertencem a KairoLibrary.dll. A correspondência nome/token/endereço está validada estruturalmente. O inventário inicial selecionou candidatos por nome; os dois overloads `_load` foram posteriormente inspecionados na rodada descrita abaixo. Os demais corpos não foram investigados nesta tarefa.

| Tipo / método | Metadata index | VA ELF |
| --- | ---: | --- |
| `kairo.unity.ui.Seb._load` | 18982 | `0x1bb0a98` |
| `kairo.unity.ui.Seb._load` | 18983 | `0x1baf5a0` |
| `kairo.unity.ui.Seb.GetMaxFrame` | 18993 | `0x1bb16bc` |
| `kairo.unity.ui.Seb.Frame` | 18994 | `0x1bb16c4` |
| `kairo.unity.ui.Seb.ConvBufferToSprite` | 19006 | `0x1bb2b5c` |
| `kairo.unity.ui.Seb.GetSpriteLocal` | 19005 | `0x1bb2290` |
| `kairo.unity.ui.ResourceManager.LoadSeb` | 18849 | `0x1ba5bfc` |
| `kairo.unity.ui.ResourceManager.LoadSeb` | 18850 | `0x1ba5d48` |
| `kairo.unity.util.JarInflater.ConvertExtension` | 13973 | `0x1bfc300` |

O tipo `kairo.unity.ui.Sprite` expõe getters FrameNo, TexId, U/V/W/H, X/Y, TransX/TransY, ReverseU/ReverseV, Blend e Color. Isso sugere propriedades a procurar no leitor, mas não comprova a posição de nenhum campo em disco. ConvertExtension é candidato para esclarecer referências `.gif` com membros `.png`; não aplicar essa substituição universalmente antes de conferir o corpo.

## Próximos testes de aceitação

1. Usar FDE/assembly para separar os dois overloads `Seb._load`, identificar fontes dos bytes, endian e cada leitura. Relacionar leituras a offsets no `copy.seb`, sem inferir valores pela imagem.
2. Conferir separadamente `hat_com.seb` e `weapon_com.seb`, depois um caso que falha na fórmula simples (`fukidasi.seb`) e o outlier `k.seb`. Exigir consumo exato de bytes e limites válidos em todas as variantes aceitas.
3. Seguir `ResourceManager.LoadSeb` e `JarInflater.ConvertExtension` para estabelecer parsing dos índices, linhas sem ID, opção `,bin`, busca compartilhada e fallback de idioma. Reclassificar as 103 referências só após essas regras.
4. Só com schema comprovado, escrever parser determinístico, rejeição de truncamentos/índices inválidos e round trip binário. Preservar campos desconhecidos; não substituir por defaults silenciosos.
5. Para fidelidade de animação, comparar sequência, duração, pivôs, orientação, blending e composição com o original. PNG válido e nomes de getters não bastam para considerar animação recuperada.

Resultado da etapa inicial: inventário e alvos concretos para análise, ainda sem parser. O avanço posterior está documentado abaixo; nenhum renderer ou comportamento aproximado foi implementado.

## Rodada nativa delimitada: entrada dos dois leitores

Após o inventário, foi executada **uma** exportação sequencial no projeto Ghidra `wizard_il2cpp_fde`, sem autoanalysis ampla. Seleção local: `build/research/seb-readers.tsv`; saídas proprietárias: `build/research/seb-readers/`. O programa mantém base Ghidra `0x100000`; os endereços abaixo já subtraem essa base.

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript InspectSelectedMethods.java build/research/seb-readers.tsv build/research/seb-readers -max-cpu 1
```

| Método | Intervalo FDE (fim exclusivo) | Bytes |
| --- | --- | ---: |
| `_load`, índice 18982 | `0x1bb0a98..0x1bb0b90` | 248 |
| `_load`, índice 18983 | `0x1baf5a0..0x1bb0a98` | 5.368 |

O overload 18982 constrói `java.io.ByteArrayInputStream` (BL em `0x1bb0ae8` para `0x1be8544`) e chama o overload 18983 em `0x1bb0af4`. Portanto o primeiro é wrapper de bytes para stream, não um segundo layout independente demonstrado.

O overload 18983 constrói `java.io.DataInputStream` em `0x1baf6d4`, destino `0x1be89e8`. O início do parsing é confirmado pelo assembly:

1. `0x1baf6fc` chama `ReadByte` (`0x1be8aa4`). `0x1baf700` faz extensão de sinal do byte; `0x1baf704` testa seu bit 7.
2. Se o bit 7 está desligado, `0x1baf710` lê outro byte; a instrução em `0x1baf718` combina os dois em ordem byte alto/baixo, e `0x1baf71c` grava modo zero.
3. Se o bit 7 está ligado, `0x1baf724` nega o primeiro byte com sinal e guarda o modo; `0x1baf734` chama `ReadShort` (`0x1be8b60`) para obter o valor seguinte. `0x1baf738` o estende com sinal.
4. `0x1baf744` chama novamente ReadShort; o resultado estendido com sinal é gravado em `this+0x1c` em `0x1baf74c`. O primeiro valor é usado para alocar duas estruturas/arrays e controlar uma iteração. Não foram atribuídos nomes semânticos definitivos a esses campos.
5. Dentro da iteração, há ReadShort em `0x1baf7b8`; quando modo zero, ocorre outra leitura em `0x1baf7cc`. Há caminhos distintos para modo zero e modo um em `0x1baf850..0x1baf858`.

Isso explica por que aplicar quatro BE16 desde offset zero ao arquivo `k.seb`, que começa por `ff`, era apenas uma sondagem: **o leitor possui um caminho de prefixo negativo**, e `ff` seleciona modo 1. Não se comprovou nesta rodada o endian do corpo de ReadShort, nem a semântica de todos os registros. A composição explícita dos dois bytes no caminho sem prefixo é confirmada; não extrapolar ao restante do arquivo.

O pseudocódigo do leitor grande contém warning PIC e referências a `0x1bb0fe4`, fora do FDE: absorveu conteúdo de destino. Não usá-lo como parser recuperado. As observações acima vêm das instruções no intervalo delimitado; todas as linhas dos dois exports assembly ficaram dentro dos respectivos FDEs. Isso verifica contenção, não cobertura integral ou correção runtime. A coordenação completa dos sprites e ConvBufferToSprite ficam pendentes.

### Conferência adicional de ReadShort

Uma segunda exportação pequena inspecionou somente `DataInputStream.ReadShort`, metadata **13514**, FDE **0x1be8b60..0x1be8bdc**, 124 bytes. Seleção e saída: `build/research/seb-readshort.tsv` e `build/research/seb-readshort/`. O corpo carrega o objeto em `this+0x38` e faz chamada virtual em **0x1be8ba8**, usando o slot `classe+0x208`. Depois transfere em **0x1be8bd4** para **System.Net.IPAddress.NetworkToHostOrder**, metadata **41497**, VA **0x23237d0**. Isso confirma uma conversão de ordem de rede explícita no caminho de ReadShort. Ainda falta resolver a implementação da leitura virtual antes dessa conversão para uma prova integral de bytes do stream até o retorno.

O pseudocódigo desse método também absorveu funções fora do intervalo e foi descartado como prova; a observação está restrita ao assembly de 31 instruções/124 bytes. Não foi implementado parser apoiado no pseudocódigo contaminado. A evidência já reduz os próximos alvos a esse slot virtual e aos registros consumidos por Seb, sem iniciar novas análises nesta etapa.

## Ciclo seguinte: parser estrutural com reconstrução exata

Implementação: [`tools/inspect_seb.py`](../tools/inspect_seb.py), somente stdlib. Testes sintéticos: [`tests/test_seb.py`](../tests/test_seb.py). Os arquivos locais não são incorporados aos testes ou ao código.

```powershell
python tools/inspect_seb.py data/extracted/manifest.json --output research/local/seb-inventory.json
python -m unittest discover -s tests -p test_seb.py -v
```

Resultado: **303/303 arquivos** passam identidade pelo manifesto, consumo exato sem trailing bytes e reconstrução byte a byte **a partir dos campos parseados**. Não há cache do payload original no objeto retornado. São **302 modo 0**, **um modo 1**, **452 grupos** e **2.686 registros**. Duas execuções do inventário produziram estruturas idênticas ao JSON salvo, SHA-256 `70b8d7c79ea294936dd41d696df1bf78d0a972ef75354fdab0ec90a00a61d0a0`.

Isso confirma a cobertura estrutural da amostra, não a fidelidade de animação. O script usa nomes neutros como `values`, `header_value`, `control`, `pair` e `tail_pair`; não atribui duração, camada ou significado de sprite apenas por posição. Modos diferentes de 0/1 são rejeitados explicitamente, não aproximados.

### Endian e leituras virtuais

O construtor de DataInputStream, em `0x1be89e8`, chama `BinaryReader..ctor` (`0x1db8e60`, metadata 9997) e guarda o objeto em `this+0x38` na instrução `0x1be8a60`. BinaryReader.ReadInt16 (10009, `0x1db9514`) solicita dois bytes ao preenchimento do buffer e usa `ldrh` em `0x1db9548`, sob ELF little-endian. NetworkToHostOrder16 usa `rev` seguido de shift de 16 bits (`0x2323814..0x232381c`). Essa cadeia sustenta a leitura **big-endian** dos shorts serializados, em vez de deduzi-la só pelo aspecto dos arquivos.

Corroboração metadata v31: BinaryReader tem vtableStart 15679; slot **13** contém `0x60004e33`, decodificado para método **10009**. DataInputStream tem vtableStart 20456; slot **9** contém `0x600069e1`, decodificado para método herdado **13552**, InputStream.Read (`0x1be9b30`). Os slots gravados nas definições dos métodos concordam. Os offsets de chamada nativa `0x208` e `0x1c8` correspondem a esses slots com uma origem comum `0x138` e stride 16; trata-se de corroboração estática, não captura do runtime. A fórmula de decodificação de índices v27+ é fixada no [Metadata.cs do Il2CppDumper](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Metadata.cs).

InputStream.Read encaminha ao stream encapsulado em `this+0x18` e à operação virtual de leitura de byte. A análise não modifica esse stream nem executa as funções. Exports adicionais locais: `build/research/seb-input-chain/` e `build/research/seb-byte-reader/`. Continuamos usando assembly; os exports C com absorção de caudas não fundamentam o parser.

### Layout consumido e preservado

`h` significa short assinado de 16 bits big-endian; `B`, byte unsigned; `I`, quatro bytes preservados como inteiro unsigned big-endian.

| Elemento | Modo 0 | Modo 1 |
| --- | --- | --- |
| Prefixo | Ausente; primeiro byte não negativo | `ff` (primeiro byte assinado -1) |
| Cabeçalho | Contagem de grupos em dois bytes; `header_value:h` | `group_count:h`; `header_value:h` |
| Início de grupo | `record_count:h`, `legacy_value:h` | `record_count:h` |
| Registro | Dez `h` consecutivos | Quatro `h`, um `B`, campos condicionais abaixo |

`legacy_value` é consumido mas seu retorno não é usado naquele ponto do leitor (`0x1baf7cc`); o encoder o preserva exatamente, sem substituí-lo pela contagem.

No modo 1, os quatro shorts iniciais vão para slots temporários **0, 1, 6, 7**; o byte seguinte para slot **10**. Chamando o slot 1 de `d` e o byte de `c`, o restante do registro é:

1. `extra_word:I` se **-6 ≤ d ≤ -2**, ou se **d ≥ 0 e c ≠ 0**. O ramo para `d` negativo é tomado antes do teste de `c`; portanto `c ≠ 0` não força esse campo quando `d=-9`, por exemplo. Evidência: `0x1baf904..0x1bafb20`.
2. `pair:2h` se **d ≥ 0 ou d=-8**, destinado aos slots 2/3. Evidência: `0x1bafb24..0x1bafb70`.
3. `tail_pair:2h` sempre, destinado aos slots 4/5. Evidência: `0x1bafb74..0x1bafbb4`.
4. `flags:B` somente se **d ≥ 0**. Os bits 0/1 alimentam slots 8/9; outros bits são preservados na representação de disco. Evidência: `0x1bafbb8..0x1bafbfc`.

O modo 0 lê dez shorts para slots 0..9 em `0x1baf918..0x1bafa50`. O código deriva slots adicionais, inclusive tratamento especial de valores negativos e cor/controle; o parser **não emula** essas transformações, pois seu escopo é o layout em disco. A estrutura não deve ser confundida com o buffer compacto de runtime.

### Organização runtime observada, ainda sem renderer

Após ler cada registro, o loader guarda slot 0 em um array de shorts e compacta outros campos em oito bytes por registro (`0x1bafc7c..0x1bafdd0`). Há caminho de valores adicionais quando dados não cabem na representação compacta; sua interpretação ainda precisa ser conferida junto a ConvBufferToSprite. Ao final, aloca/grava arrays em `this+0x20`, `this+0x28`, `this+0x30`, copia buffers dos grupos e acumula offsets (`0x1bb00bc..0x1bb01b8`). Isso sustenta a separação grupo/registro do arquivo, mas não autoriza nomes como “camada” ou “quadro” para cada array.

Não foram implementados buffers runtime, cálculo de bounding box, clipping, blending, avanço de animação ou efeitos especiais de registros negativos. Próxima análise funcional: ConvBufferToSprite/GetSpritesLocal e getters correspondentes, conectando cada campo a operações de apresentação antes de desenhar qualquer frame.

### Testes e comportamento de rejeição

Seis testes sintéticos cobrem valores signed extremos, preservação do short não usado, estruturas vazias, oito combinações nos limites dos condicionais de modo 1, todos os truncamentos de dois fixtures (modos 0 e 1), bytes excedentes, modo desconhecido, contagens negativas e campos opcionais inconsistentes no encoder. Nenhum fixture contém dados proprietários.

O parser rejeita contagens de alocação negativas, truncamento ou bytes não consumidos. Ele não procura um layout alternativo para fazer o arquivo “passar”. O CLI verifica hashes/tamanhos antes de analisar, impede caminhos de membros fora da raiz e não permite gravar seu relatório na árvore dos assets de entrada. O round trip é uma garantia de preservação estrutural; a evidência de código e consumo total são necessárias para interpretar esse resultado, e ainda faltam comparações de apresentação com o jogo.
