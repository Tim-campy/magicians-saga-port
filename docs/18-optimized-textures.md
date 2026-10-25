# M0 — Mapeamento OPT de texturas otimizadas

Data: 2026-10-25. Mesma identidade APK/ELF dos relatórios anteriores. O teste de [16](16-sprite-layout.md) mostrou que crops SEB não podem ser aplicados diretamente a todo PNG. Este ciclo identifica a estrutura OPT, seu consumidor e limites da reconstrução visual. Não há renderer do jogo nem comparação runtime.

## Parser e cobertura

[`tools/inspect_opt.py`](../tools/inspect_opt.py) lê e reconstrói a variante regular OPT. [`tests/test_opt.py`](../tests/test_opt.py) usa somente fixtures sintéticas. O parser SEB e o extrator não foram modificados.

```powershell
python tools/inspect_opt.py data/extracted/manifest.json --output research/local/opt-inventory.json
python -m unittest discover -s tests -p test_opt.py -v
```

**286/286 OPT**: hashes/tamanhos do manifesto conferidos, consumo total e round trip exato a partir de escalares; **38.437 bytes**, **2.559 células**, **2.481 registros de peças**. Os **286 OPTINFO** foram catalogados como texto UTF-8. O JSON foi idêntico em duas execuções, SHA-256 `d35c1bf57e7d49421945a7ed55f8817fba16955e995bd39b8944cfa2e5baff97`. Manifesto de entrada SHA-256 `7a30b5c9df677d9e328e6ab10ffb8bc080940f8e7f46e1f5f225c9ea36ae13f4`.

Há uma variante OPT com primeiro byte `ff` no leitor nativo, mas **nenhuma** das 286 amostras a usa. Ela é rejeitada explicitamente pelo parser, sem adivinhar layout completo ou aceitar bytes residuais. Todas as amostras pertencem à variante regular.

## Evidência nativa

Ghidra foi executado sequencialmente com FDEs e sem análise ampla. Seleções/exports locais: `opt-readers`, `opt-geometry`, `opt-consumer`, `opt-endian` e `opt-dimensions` sob `build/research/`. Base Ghidra `0x100000`; endereços no relatório são **VAs ELF**.

| Método | Metadata index | VA ELF | Bytes FDE |
| --- | ---: | --- | ---: |
| Image.LoadOptimize, stream | 18384 | `0x1b8d2dc` | 4.304 |
| Image.LoadOptimize, byte array | 18385 | `0x1b8e3ac` | 260 |
| Image.GetOptimize, seleção | 18387 | `0x1b8e4c0` | 156 |
| Image.GetOptimize, expansão | 18390 | `0x1b8e55c` | 600 |
| Image.GetOptimizeSeb | 18391 | `0x1b8e7d4` | 436 |
| Graphics.DrawImage | 17871 | `0x1b63574` | 584 |
| Graphics._drawImage | 17876 | `0x1b637bc` | 5.332 |

LoadOptimize do array constrói ByteArrayInputStream e chama o overload stream. Os pseudocódigos novamente absorvem caudas/callees; as conclusões abaixo usam assembly delimitado, nomes do catálogo e dados locais. Não considerar status `complete` do decompilador como prova de recuperação semântica completa.

### Layout em disco

| Campo | Encoding | Observação |
| --- | --- | --- |
| Passo horizontal da célula | byte unsigned | Guardado em `Image+0xc0`; `ff` seleciona outra variante |
| Passo vertical da célula | byte unsigned | Guardado em `Image+0xc2` |
| Colunas | byte signed, não negativo | Campo `Image+0x98` |
| Linhas | byte signed, não negativo | Campo `Image+0x9c` |
| Para cada célula, em ordem linha/coluna | byte signed de contagem | Contagem de peças, não negativa |
| Para cada peça | sete shorts signed big-endian | 14 bytes por registro |

No loader, os passos são inicialmente estendidos com sinal, mas depois recarregados como byte unsigned e gravados novamente (`0x1b8d470..0x1b8d488`). **Zero não é convertido em 256 nesse caminho.** Colunas/linhas e contagens de peças são estendidas com sinal; valores negativos causariam alocações inválidas, portanto o parser os rejeita.

As sete leituras repetidas passam por StreamUtil.ReadShort (`0x1c0c42c`), que solicita dois bytes e chama EncodeUtil.ByteAryToShort com argumento 1. Este chama ConvertEndian com tamanho 2; o assembly troca os dois bytes e BitConverter.ToInt16 usa `ldrh` no ELF little-endian. Isso confirma shorts **big-endian**, não apenas um ajuste visual aos valores.

GetOptimize calcula **`((columns × row) + column) × maxPieces + pieceIndex`** em `0x1b8e4cc..0x1b8e4dc` (e caminho equivalente para índices shorts). A sequência de arquivos é, portanto, **row-major**. Slots não usados são representados por sentinela no mapa runtime, e o getter retorna nulo. O parser preserva células vazias como vazias; não inventa uma peça transparente.

### Semântica das sete posições

Image.GetOptimize expande cada registro para sete inteiros na mesma ordem. O consumidor Graphics._drawImage confirma:

| Posição | Papel demonstrado |
| --- | --- |
| 0 | Seletor da imagem fonte: **-1 usa a própria imagem**; outro valor indexa array em `Image+0xb8` |
| 1 | Deslocamento horizontal da peça dentro da célula lógica |
| 2 | Deslocamento vertical da peça dentro da célula lógica |
| 3 | Origem horizontal do recorte na imagem fonte |
| 4 | Origem vertical do recorte na imagem fonte |
| 5 | Largura do recorte/peça |
| 6 | Altura do recorte/peça |

Evidências: chamada GetOptimize em `0x1b64080`; escolha de imagem em `0x1b64094..0x1b640cc`; subtração do início da célula e dos deslocamentos das posições 1/2 em `0x1b6412c..0x1b64180`; limites de largura/altura em `0x1b64174..0x1b64200`; soma das posições 3/4 ao recorte ajustado em `0x1b64290..0x1b642a0`.

O posicionamento de uma peça na grade é, assim, `x = column × cell_width + offset_x`, `y = row × cell_height + offset_y`. Graphics.DrawImage percorre linhas/colunas com esses passos e chama o consumidor por célula (`0x1b6367c..0x1b636dc`). O consumidor também faz clipping e aplica escala/estado gráfico; uma cópia de referência de pixels não implementa esses efeitos.

## Ligação aos PNGs e validação geométrica

Todos os 286 OPT possuem PNG de mesmo basename no mesmo contêiner, com hash/tamanho conferidos. **Todos os 2.481 registros usam seletor -1**. Aplicando posições 3/4/5/6 como retângulo, **2.481/2.481 recortes estão dentro do PNG pareado**. O relatório CLI registra cada par, dimensões IHDR e contagem de recortes válidos; isso é correspondência estática, não captura do carregamento runtime.

Exemplo `hat_000`: PNG físico **39×15**, OPT com célula **30×30**, grade **1×2**. A primeira peça contém `(-1,4,4,20,0,19,15)`; a segunda, `(-1,4,5,0,0,20,15)`. Portanto a grade distribui duas regiões do PNG em células distintas com padding, em vez de usar o PNG físico como uma imagem lógica de 30×60 pronta. Isso explica por que o crop SEB direto excedia o arquivo físico. A associação de cada instância SEB à imagem certa ainda precisa da cadeia ResourceManager/INF.

Não foram encontradas sobreposições entre retângulos de destino dos 2.481 registros, considerando sua posição na grade. Isso evita escolher uma regra de blending entre peças para um futuro harness dessa amostra; não demonstra a regra gráfica geral para arquivos com sobreposição.

## Dimensão lógica: exceção que não deve ser mascarada

**268/286 OPT** têm todas as peças dentro do retângulo `cell_width×columns` por `cell_height×rows`. **18**, todos do contêiner `19_chip`, extrapolam: IDs 0–3 são `chip_00_0` até `chip_00_3`; IDs 4–17 são os pares `chip_01_1/3` até `chip_07_1/3`. A classificação anterior de todos como `chip_00_*` estava incorreta. Exemplos têm header **112×128**, grade **1×1**, mas uma peça como `(-1,226,127,0,0,398,393)` alcança regiões muito maiores.

Os OPTINFO desses arquivos citam `optimize_624x640.inf`. Embora 624/640 tenham bytes baixos 112/128, **não** há prova de que o runtime deva somar 512 ou recuperar dimensão pelo nome da configuração. Não usar essa coincidência para “corrigir” o header. Image.GetWidth/GetHeight e seus getters retornam os campos **+0x28/+0x2c**, distintos dos passos OPT +0xc0/+0xc2; o caminho que define essas dimensões completas ainda não foi mapeado.

Portanto `header×grid` está comprovado como domínio de iteração desse DrawImage, não como dimensão original completa de toda imagem. O harness abaixo registra esse domínio separadamente e calcula um canvas de **occupied bounds** pela união dos retângulos. Ele não chama esse canvas de dimensão original nem trunca regiões excedentes. O modo opcional `--strict-grid` rejeita os 18 casos que extrapolam.

## OPTINFO e limitações

Todos os 286 OPTINFO observados têm cinco linhas UTF-8: marcador `img1`, nome de configuração, `palette=off`, linha `1` e uma linha de registro contendo nome PNG/data. O inventariador só cataloga tag e referência de configuração; nenhuma prova de consumo runtime desse arquivo foi encontrada nesta tarefa. Datas de produção não são timestamps de execução nem parte de regras do jogo.

Cinco testes sintéticos verificam grade/ordem, shorts signed extremos, dimensões unsigned, múltiplas peças, células vazias, todos os truncamentos de um fixture, bytes excedentes, prefixo FF não suportado, contagens negativas e invariantes de encoder. Três testes adicionais do harness verificam placement row-major, origem negativa, padding, recorte, preservação de alpha/RGB invisível e rejeição de overlap/crop inválido/seletor externo. **Oito testes passam**; nenhum contém dados proprietários. O parser/encoder estrutural não implementa a compactação runtime ou variante FF.

## Harness visual: occupied bounds, sem dimensão original presumida

[`tools/render_opt_reference.py`](../tools/render_opt_reference.py) é um **harness de pesquisa**, separado do extrator final e do renderer do jogo. Usa Pillow instalado localmente em `.tools/unitypy-libs`; nesta execução, versão 13.0.0. Saídas são permitidas somente em subdiretório de `build/research`, separado dos assets de entrada.

```powershell
python tools/render_opt_reference.py data/extracted/manifest.json --output build/research/opt-reference
python tools/render_opt_reference.py data/extracted/manifest.json --output build/research/opt-reference-hat --select hat_000.opt --strict-grid
```

Para cada OPT, o harness valida hashes de OPT/PNG, calcula retângulos por `column×cell_width+dx`, `row×cell_height+dy`, verifica os recortes físicos e rejeita sobreposições. O canvas é exatamente o retângulo delimitador das peças, com sua origem original registrada — inclusive se negativa. Não inclui automaticamente a origem zero nem padding externo para imitar uma imagem lógica. Isso permite visualizar também as 18 amostras chip sem fingir que seu domínio de header foi resolvido.

A cópia é **RGBA exata, sem máscara e sem alpha compositing**: bytes RGB de pixels com alpha zero são preservados dentro das peças. Regiões do canvas não cobertas por peça recebem RGBA zero como convenção de visualização, registrada no relatório; isso não prova o conteúdo original dessas regiões. Não é escolhida regra de blending ou overwrite para overlap: qualquer overlap faz o harness recusar a imagem.

Cada peça é recortada novamente do canvas e comparada byte a byte com seu crop no PNG packed. Depois de salvar, o PNG é reaberto e seus bytes RGBA são comparados com o canvas em memória. Esse teste valida a movimentação dos pixels, **não** a equivalência ao frame do jogo.

Resultados de `build/research/opt-reference/report.json`:

| Medida | Resultado |
| --- | ---: |
| Referências PNG por occupied bounds | 286 |
| Peças verificadas por recorte reverso | 2.481 |
| Referências com peça fora do domínio header×grid | 18 |
| Pares de retângulos sobrepostos | 0 |
| Pixels atribuídos a peças | 5.272.514 |
| Pixels atribuídos com alpha não zero | 3.170.722 |
| Pixels atribuídos com alpha zero | 2.101.792 |
| Pixels de padding não atribuídos dentro dos bounds | 2.120.364 |

As medidas de cobertura/padding não são estimativas de tamanho lógico original. Duas execuções produziram o mesmo relatório, incluindo hashes dos PNGs salvos: SHA-256 do relatório `ebeeefea8be8ae1711832f2a5cfdbe740df7a98dde6bd04955b3d1aceef65477`.

Inspeção visual local de duas referências confirmou que os arquivos são legíveis: `members/00_hat/0175_hat_000.occupied.png` tem bounds **[4,4,24,50]**, canvas **20×46**; `members/19_chip/0036_chip_00_0.occupied.png` tem bounds **[226,127,624,520]**, canvas **398×393**. Essa inspeção não é comparação com o original em execução. Todos os 286 PNGs, inclusive armas e demais variantes, passaram pela prova de bytes descrita acima.

Próximos passos: estabelecer a origem dos campos Image+0x28/+0x2c, resolver ResourceManager/INF e relacionar geometria SEB a OPT/imagem efetivamente selecionada. Timing, composição de cena, alpha blending do jogo e renderer nativo continuam pendentes.
