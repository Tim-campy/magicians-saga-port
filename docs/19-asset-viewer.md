# Visualizador técnico nativo de PNG e OPT

Data: 2026-10-25. `asset_viewer` abre uma imagem PNG local usando stb_image e
SDL3, somente no módulo desktop. É uma ferramenta de inspeção de recursos:
não monta uma cena do jogo nem implementa menus, animações ou gameplay.
O incremento OPT abaixo compõe somente as peças de uma imagem packed em um
canvas de pesquisa por occupied bounds, sem presumir dimensão original.

## Build opcional e dependências fixadas

`SAGA_BUILD_VIEWER=OFF` por padrão. O leitor `asset_inspect`, sua biblioteca e
seus testes continuam sem dependências externas. Habilitar o visualizador
baixa fontes oficiais e compila SDL localmente; não instala nada globalmente.

| Dependência | Versão/origem | SHA-256 |
| --- | --- | --- |
| SDL3 | [release-3.4.18, tar.gz oficial](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18) | `9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3` |
| stb_image.h | [commit 003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5](https://github.com/nothings/stb/blob/003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5/stb_image.h) | `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3` |

CMake verifica ambos os hashes. O header permanece em `build/_deps/stb`;
SDL usa `FETCHCONTENT_BASE_DIR` (padrão deste projeto `build/_deps`; o cache
inicial desta sessão já apontava para `build/viewer/_deps`). Nenhum header,
fonte ou binário vendor foi copiado para `src/`.

SDL usa licença zlib, incluída em `sdl3-src/LICENSE.txt`. stb_image inclui no
próprio header as alternativas MIT e domínio público. Preservar os avisos
aplicáveis em eventual distribuição; esta sessão produziu apenas build local.

Windows com Visual Studio C++ instalado:

```bat
tools\build-viewer.cmd
build\viewer\asset_viewer.exe data\extracted\members\CAMINHO_DA_IMAGEM.png
```

O helper usa `vswhere`, configura ambiente apenas no processo filho, compila
com quatro jobs e roda CTest. O modo janela permite redimensionar, usa nearest
neighbor e letterbox; Esc ou fechar a janela encerra. O modo janela foi
compilado, mas sua execução interativa não foi validada nesta sessão.

Linux, instruções de build **não executadas**:

```sh
cmake -S . -B build/viewer -DSAGA_BUILD_VIEWER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/viewer --parallel 4
ctest --test-dir build/viewer --output-on-failure
```

SDL pode exigir bibliotecas de desenvolvimento dos backends da distribuição
Linux; elas não foram instaladas ou verificadas aqui. A fonte desktop usa
APIs SDL portáveis, mas isso não constitui validação de um build Linux.

## Captura sem janela

```bat
build\viewer\asset_viewer.exe imagem.png --headless --capture captura.bmp
```

`--headless` exige `--capture` e vice-versa. O destino não pode existir; o
programa recusa substituição. `SDL_Init(0)`, uma superfície RGBA32 e
`SDL_CreateSoftwareRenderer` permitem renderizar sem inicializar vídeo ou
abrir janela. A textura usa `SDL_SCALEMODE_NEAREST` e `SDL_BLENDMODE_NONE`;
o resultado 1:1 é salvo em BMP com alpha. Essa escolha preserva os bytes RGBA
para comparação, inclusive RGB de pixels transparentes, em vez de compor o
asset sobre um fundo. A [API SDL](https://wiki.libsdl.org/SDL3/SDL_CreateSoftwareRenderer)
documenta o renderer associado à superfície.

Entrada: PNG estático de até 64 MiB, máximo de 8.192 pixels por eixo e
16.777.216 pixels totais. O resultado interno é RGBA8. PNG de 16 bits por
canal é rejeitado, evitando redução silenciosa de precisão. Não há suporte a
animação APNG, interpretação genérica de atlas, color management ou reconstrução de
materiais/shaders Unity. O decoder foi compilado apenas com suporte PNG.

`SDL_main.h` fornece os argumentos UTF-8; o leitor usa
`std::filesystem::u8path`. Entrada e captura contendo acento e caractere
suplementar foram testadas no Windows. Nenhuma API SDL está no núcleo de
assets. A captura é um artefato do renderer software, não uma captura do jogo
Android ou da tela do desktop.

## Evidências de validação

Windows x64, MSVC 19.51.36252.0, CMake 4.3.1-msvc1, Ninja 1.13.2:

- Build opcional passou; CTest do leitor passou 1/1. Log local
  `build/research/viewer-build.log`.
- Build padrão com visualizador desabilitado também passou sem exigir SDL;
  `build/research/desktop-viewer-disabled.log`.
- Fixture sintético RGBA 7×5, com cores distintas e alpha 0, 1, 127, 254 e 255:
  todos os pixels BMP resultantes são idênticos ao PNG lido por Pillow.
- `title.png` extraído da cópia local, 600×320: mesma comparação byte a byte
  RGBA passou. Artefatos locais:
  `build/research/viewer-validation/title-capture.bmp` e preview
  `title-capture.png`. Preview inspecionado visualmente: logotipo japonês,
  chamas e cajado aparecem legíveis; a forma coincide com o asset, sem
  alegação de tela/menu completo.
- Resultados e hashes dos pixels:
  `build/research/viewer-validation/results.json`.
- Repetição com nome de saída contendo `á` e `😀` passou; fixture de entrada
  também contém esses caracteres. No modo PNG simples, stdout identifica
  dimensões e conclusão; o modo OPT posterior emite JSON geométrico.
- Destino existente, assinatura inválida e PNG 16-bit foram rejeitados com
  código 1; sem argumentos retorna código 2.

O teste de pixels usa `Image.open(source).convert('RGBA').tobytes()` e o mesmo
para o BMP capturado, exigindo igualdade de dimensões e bytes. Nenhum asset
proprietário foi adicionado ao código ou baixado externamente. Saídas e
dependências estão sob `build/`, ignorado pelo controle de versão.

Limites: não foram testados reprodução de áudio, fontes, múltiplos sprites,
timing, GPU/window backend, Android, Linux ou 3DS. Este resultado valida apenas
o caminho local PNG → RGBA → textura SDL → superfície software → BMP para os
dois casos examinados. Ainda não satisfaz M1 audiovisual completo.

## Incremento OPT: composição técnica por occupied bounds

O argumento `--opt arquivo.opt` usa `src/assets/opt_grid` sem alterar o parser.
A geometria é a demonstrada em [18](18-optimized-textures.md): células em
row-major, origem `coluna × cell_width`, `linha × cell_height`, somada aos
valores 1/2 de cada peça. Valores 3/4 são a origem do crop, 5/6 são largura e
altura. Somente seletor -1 (valor 0, a própria imagem fornecida) é aceito.
Associação de imagem é responsabilidade do chamador; o viewer não resolve
ResourceManager/INF ou escolhe automaticamente um atlas.

```bat
tools\build-viewer.cmd
build\viewer\asset_viewer.exe data\extracted\members\00_hat\0003_hat_000.png --opt data\extracted\members\00_hat\0175_hat_000.opt --headless --capture build\research\hat-occupied.bmp
```

Sem `--headless/--capture`, o mesmo modo é disponível na janela técnica; esse
caminho continua apenas compilado, sem validação interativa nesta sessão.

O canvas é a união dos retângulos não vazios de destino: `[x0,y0,x1,y1]`.
Sua origem pode ser negativa ou diferente de zero. Cada destino SDL é
transladado por `(-x0,-y0)`; não se acrescenta padding externo até a origem
zero e não se corta nada pelo domínio do header. Todas as lacunas internas
recebem RGBA zero, convenção do harness. **Nenhuma dessas dimensões é
apresentada como dimensão original da imagem ou da cena.**

As peças usam `SDL_RenderTexture` com retângulos fonte/destino 1:1, nearest
e `SDL_BLENDMODE_NONE`. Preservam inclusive RGB de pixels cujo alpha é zero.
Semântica de blending do jogo não é inferida: retângulos sobrepostos são
rejeitados. A renderização mantém a fonte PNG intacta.

Validações antes de criar recursos SDL:

- Seletor externo, crop negativo/fora do PNG e OPT inválido são rejeitados.
- Peças de área zero não contribuem aos bounds; nenhuma peça não vazia causa
  erro explícito (o harness Python representa esse caso por ausência de imagem).
- Coordenadas são calculadas em `int64_t` antes das somas/produtos, com checagem
  do domínio `int32_t`; só os destinos normalizados dentro do canvas são
  convertidos para float SDL. Os campos do formato não conseguem causar
  overflow dessa aritmética ampla.
- Orçamento do viewer: 65.536 peças, 8.192 pixels por eixo e 16.777.216 pixels
  no canvas; o arquivo OPT conserva o limite de 32 MiB do parser.
- Uma máscara de ocupação de um byte por pixel detecta overlap. O algoritmo
  não compara todos os pares de retângulos: tempo proporcional às peças mais
  pixels visitados, limitado pelo canvas até sucesso ou primeira colisão.
  A soma de áreas é limitada antes de visitar cada peça.

No modo OPT, stdout é JSON com `canvas_kind`, `occupied_bounds`, `canvas_size`,
`grid_domain`, `piece_count`, `pieces_outside_grid`, `assigned_pixels` e
`padding_convention`. `grid_domain` registra separadamente o domínio de
iteração do header. `pieces_outside_grid` é um fato, não motivo para truncar
ou alterar o header. Erros continuam em stderr e retornam código 1.

### Diferencial completo executado no Windows

Harness local reproduzível: `python build/research/viewer-opt/validate.py`.
Ele cria um novo diretório `run-*`, verifica hashes de OPT, PNG packed e PNG de
referência, executa o binário com timeout por arquivo e compara contra
`build/research/opt-reference/report.json` e os PNGs daquele harness Python.

**286/286 composições e 2.481 peças coincidiram exatamente**: todos os bytes
RGBA, dimensões de canvas, origem/bounds, domínio de grid, quantidade de peças,
área atribuída e contagem de peças externas. Isso inclui os **18 arquivos**
cuja geometria extrapola o domínio do header. Nenhuma exceção foi mascarada.
Relatório local: `build/research/viewer-opt/report.json`; log de build/CTest:
`build/research/viewer-opt-build.log`. CTest **3/3** passou.

Fixture sintético também comprovou origem `[-2,-1]`, canvas 5×6, células
vazias, padding RGBA zero e preservação de cores com alpha 0/1/127/255.
PNG, OPT e BMP com nomes contendo `á` e `😀` passaram. Testes negativos
confirmaram rejeição de seletor externo, crop inválido, overlap, grade vazia,
somente peças de área zero, extensão excessiva, orçamento de pixels e
orçamento de peças. Os dois testes anteriores de PNG simples passaram
novamente com os mesmos hashes RGBA.

Previews de dois BMPs foram inspecionados: `hat_000` tem bounds
`[4,4,24,50]` e canvas 20×46; `chip_00_0` tem bounds `[226,127,624,520]`
e canvas 398×393. Imagens e caminhos completos constam no relatório local.
São peças/atlas reorganizados para pesquisa, não frames observados do jogo.

Não foi introduzido renderer de cena, animação, timing, dimensões originais,
clipping do jogo ou blending. Linux, 3DS e backend GPU/janela continuam sem
execução validada. O resultado é equivalência com o harness geométrico Python
para a amostra disponível, não equivalência audiovisual ao Android.
