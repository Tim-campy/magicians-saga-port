# M0 — Layout geométrico dos sprites SEB

Data: 2026-10-25. Análise estática do mesmo APK/ELF identificados em [07](07-il2cpp-code-map.md), após o parser estrutural de [13](13-animation-formats.md). Nenhum parser foi alterado, nenhum renderer foi implementado e nenhum frame foi comparado com execução do jogo.

## Evidência e método

Duas exportações Ghidra sequenciais, sem autoanalysis ampla, usaram `InspectSelectedMethods.java` e intervalos FDE do ELF. Projeto local: `wizard_il2cpp_fde`; seleções: `build/research/sprite-layout.tsv` e `sprite-fields.tsv`; saídas proprietárias nos diretórios homônimos. Os endereços abaixo são **VAs ELF**, enquanto os exports Ghidra somam base `0x100000`.

| Método | Metadata index | VA ELF | Bytes FDE |
| --- | ---: | --- | ---: |
| Seb.ConvBufferToSprite | 19006 | `0x1bb2b5c` | 828 |
| Seb.GetSpritesLocal | 18998 | `0x1bb0b90` | 372 |
| Seb.GetSpriteLocal | 19005 | `0x1bb2290` | 2.220 |
| Seb.GetImage | 18972 | `0x1baf470` | 16 |

Também foram exportados os getters do tipo `kairo.unity.ui.Sprite`. Seus nomes e retornos ligam os slots do array de inteiros a identificadores originais. Os pseudocódigos de GetSpritesLocal e ConvBufferToSprite incluem trechos fora de seus FDEs (o segundo inclusive após o caminho de exceção, sem warning inicial evidente). Por isso, as afirmações abaixo usam assembly delimitado e nomes do mapa; ausência de warning não torna todo pseudocódigo confiável.

## Slots nomeados pelos getters

O array retornado/manipulado usa dados de elementos a partir do offset `0x20`. `Sprite` envolve uma referência a esse array. Os nomes abaixo são os nomes **originais dos getters**, não nomes inferidos por aparência do conteúdo.

| Slot int32 | Offset no array | Getter original | VA ELF do getter |
| --- | --- | --- | --- |
| 0 | `0x20` | FrameNo | `0x1ba4f10` |
| 1 | `0x24` | TexId | `0x1ba0c10` |
| 2 | `0x28` | U | `0x1ba4f38` |
| 3 | `0x2c` | V | `0x1ba4f64` |
| 4 | `0x30` | W | `0x1ba4f90` |
| 5 | `0x34` | H | `0x1ba4fbc` |
| 6 | `0x38` | X e TransX | `0x1ba4fe8`, `0x1ba5040` |
| 7 | `0x3c` | Y e TransY | `0x1ba5014`, `0x1ba506c` |
| 8 | `0x40` | ReverseU | `0x1ba5098` |
| 9 | `0x44` | ReverseV | `0x1ba50c4` |
| 10 | `0x48` | Blend | `0x1ba50f0` |
| 11 | `0x4c` | Color | `0x1ba511c` |

X/TransX são aliases do mesmo slot, assim como Y/TransY. Isso permite nomear campos de geometria com confiança estática; não fixa ainda unidade, origem da tela, escala de viewport ou a semântica completa de valores negativos em TexId.

## Conversão do buffer compacto

ConvBufferToSprite recebe `this`, índice de registro, array de saída e um argumento de reversão. O array `this+0x28` contém shorts; o elemento selecionado é estendido com sinal e colocado em FrameNo. `this+0x30` contém oito bytes por registro.

No caso compacto normal, o primeiro byte é TexId **signed**. Os bytes seguintes são X e Y **signed**, depois U, V, W e H **unsigned**. O último byte fornece Blend no nibble inferior, ReverseU no bit 6 e ReverseV no bit 5. A sequência é observável nas cargas e stores de `0x1bb2d18..0x1bb2dcc` e corresponde à compactação já descrita em [13](13-animation-formats.md).

Se o primeiro byte é **-128**, ele é um marcador de representação estendida, não o TexId final. O byte seguinte indexa sete shorts do array `this+0x38`: TexId, X, Y, U, V, W, H, todos estendidos com sinal. Quatro bytes do buffer compacto recompõem Color em ordem alto→baixo; flags/Blend continuam no último byte. Esse caminho é necessário para valores que não cabem nos campos compactos e não pode ser descartado como um sprite vazio.

Não confundir layout do buffer runtime com layout em disco: o modo 0 do arquivo tem dez shorts por registro, mas o loader pode escolher essa representação estendida e derivar Color/Blend para comandos especiais. A semântica de todos esses comandos ainda está pendente.

## Reversão e deslocamento

Em `0x1bb2dd0..0x1bb2e80`, o método combina o bit 0 do argumento de reversão com o bit 1 do byte `this+0x40`. Se diferem, inverte ReverseU e ajusta X. O bit 0 de `this+0x40` controla a inversão vertical e ReverseV.

Para o caminho comum (TexId diferente de -8), a fórmula observada tem arredondamento inteiro: para largura positiva, **X' = -2 × floor(W/2) - X**; analogamente **Y' = -2 × floor(H/2) - Y**. Por exemplo, largura ímpar 5 usa deslocamento -4, não -5. O assembly contém redução para signed16 e operações de shift; uma implementação futura deve preservar esses detalhes e também testar os limites signed, não substituir diretamente por uma reflexão geométrica em ponto flutuante.

TexId **-8** recebe tratamento separado: no caminho horizontal, X, U e W são negados; no vertical, Y, V e H são negados. Isso prova que valores negativos de TexId podem representar casos especiais; não podem ser usados automaticamente como índices de PNG. O significado de -8 não foi nomeado nesta etapa.

## Seleção de registros por FrameNo

GetSpritesLocal usa o comprimento do array `this+0x20`, que corresponde à quantidade de grupos do loader, obtém armazenamento de trabalho e chama GetSpriteLocal para cada índice de grupo, passando o mesmo argumento de seleção. O BL para GetSpriteLocal está em `0x1bb0c5c`. O retorno pode conter referências nulas; não interpretar todos os grupos como imagens sempre visíveis.

GetSpriteLocal normaliza o argumento por resto de **10.000** (`0x1bb233c..0x1bb235c`) e usa comparações com os shorts de FrameNo dentro do intervalo do grupo (`0x1bb2608..0x1bb2644`). Há busca pelo registro correspondente, cache e um caminho para valores entre registros. Na correspondência exata, chama ConvBufferToSprite em `0x1bb27ec`; passa reversão baseada em comparação do argumento original com 9.999. Outro caminho converte dois registros adjacentes (`0x1bb2854` e `0x1bb286c`) antes de combinar campos; a regra completa dessa combinação ainda não foi auditada.

Isso identifica FrameNo como coordenada de seleção interna, **não** segundos, ticks, FPS ou duração de animação. Faltam os chamadores que avançam esse argumento, regras de looping e a análise completa do caminho intermediário. O relatório não atribui timing por nomes de métodos.

## Ligação a imagem: evidência parcial

Seb.GetImage lê a referência em `this+0x88` e, se não nula, transfere para `ResourceManager.GetImage`, índice 18897, VA **0x1ba9a98**. O corpo do ResourceManager e a resolução de índices INF não foram seguidos nesta etapa. Portanto, a cadeia TexId→entrada de imagem→PNG/atlas continua parcial.

Há uma amostra pequena consistente para o próximo teste: no contêiner `load`, `img.inf` registra ID 0 para `copy.png` e ID 1 para `gauge.png`. O único registro de `copy.seb` contém:

`FrameNo=0, TexId=0, U=0, V=0, W=82, H=20, X=-41, Y=0, ReverseU=0, ReverseV=0`.

O IHDR de `copy.png` mede **82×20**; `gauge.png`, **124×60**. A coincidência entre ID, nome e dimensões é evidência de correspondência nessa amostra, mas ainda não valida regras universais de atlas, fallback de idioma, substituição `.gif`→`.png` ou opções `,bin`. O crop U/V/W/H e o deslocamento X/Y estão nomeados pelo código; sua apresentação final exige seguir o consumidor gráfico.

## Próximos testes

1. Seguir ResourceManager.GetImage e a carga dos índices INF; comprovar TexId→imagem para `load` antes de generalizar para contêineres com referências não literais.
2. Conferir consumidor de Sprite.Render/RenderLayer para origem do crop, destino, escala, blending e significado dos comandos TexId negativos.
3. Construir casos de referência com largura/altura ímpares, flags de reversão e representação estendida; comparar operações inteiras e geometria contra assembly e, depois, contra o jogo.
4. Resolver a combinação entre registros em GetSpriteLocal e os chamadores que avançam FrameNo antes de implementar interpolação ou timing.

Limite desta entrega: mapa estático de campos, compactação, reversão e seleção. Nenhum PNG foi alterado/exportado novamente e nenhum comportamento de animação foi aproximado.

## Complemento sem JVM: por que crop direto não é geral

Foi feita uma conferência exclusivamente de arquivos/metadata, sem nova análise Ghidra. Para cada registro SEB, a hipótese testada foi: interpretar TexId não negativo como ID da linha de `img.inf` na raiz do mesmo contêiner e U/V/W/H como retângulo direto no PNG literal indicado. Essa hipótese simples **não cobre o conjunto**:

| Resultado | Registros |
| --- | ---: |
| Total | 2.686 |
| TexId negativo, excluído da hipótese | 310 |
| TexId não negativo | 2.376 |
| ID sem entrada no `img.inf` raiz | 46 |
| Nome indicado sem membro literal no contêiner | 47 |
| Nome indicado corresponde a PNG local | 2.283 |
| Retângulo inteiramente dentro desse PNG | 1.515 |
| Retângulo fora dos limites desse PNG | 768 |

Não se conclui que esses 768 registros sejam inválidos. A hipótese ignora índices localizados, remapeamentos, imagens otimizadas e ajustes runtime. Exemplos concretos: `hat_com.seb` pede retângulos 30×30 (inclusive V=30), mas `hat_000.png` tem **39×15**; `weapon_com.seb` contém regiões 40×40 com U=0/40/80/120, enquanto `m_weapon_a00.png` tem **33×20**. Um renderer que recortasse diretamente esses PNGs estaria presumindo comportamento não sustentado.

O manifesto contém **286 `.opt` e 286 `.optinfo`**. `hat_000.opt` começa por `1e 1e 01 02`; o `.optinfo` correspondente nomeia `optimize_30x30.inf` e `hat_000.png`. `m_weapon_a00.opt` começa por `28 28 04 01`; seu `.optinfo` nomeia `optimize_40x40.inf`. Esses pares e dimensões são pistas de remapeamento/otimização, mas o formato não foi decodificado e os quatro bytes não receberam nomes definitivos.

Novos alvos metadata/mapa, todos `kairo.unity.ui.Image` em KairoLibrary.dll:

| Método | Índice | VA ELF |
| --- | ---: | --- |
| LoadOptimize | 18384 | `0x1b8d2dc` |
| LoadOptimize | 18385 | `0x1b8e3ac` |
| get_HasOptimize | 18386 | `0x1b8e4b0` |
| GetOptimize | 18387 | `0x1b8e4c0` |
| GetOptimizeSeb | 18391 | `0x1b8e7d4` |

Próximo experimento prioritário: seguir LoadOptimize e GetOptimizeSeb para ligar `.opt`, imagem física e coordenadas SEB, antes de generalizar o exemplo simples `copy.png`. Esta entrega não executou esses métodos nem ampliou o parser para `.opt`.
