# Image: dimensões e resolução de recursos

Estado: investigação estática parcial. As dimensões lógicas originais dos 18 recursos do contêiner `19_chip` que extrapolam a grade OPT continuam desconhecidas. Seus campos `Image+0x28/+0x2c` representam dimensões do PNG/Texture no caminho de carregamento confirmado abaixo; chamá-los previamente de dimensão lógica completa era uma hipótese não sustentada. O canvas de referência de [18-optimized-textures.md](18-optimized-textures.md) continua sendo a união dos retângulos ocupados, não uma reconstrução da dimensão original.

## Artefato e método

APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços abaixo são VAs do ELF. Os exports Ghidra somam `0x100000`.

`tools/ghidra/ExportBoundedInstructions.java` exporta assembly linear exclusivamente dentro dos intervalos fornecidos pelo inventário FDE, sem decompilador e sem seguir fluxo fora deles. Confere alinhamento, memória executável e inicializada, registra hash da biblioteca, base, limites, hash dos bytes e cobertura. Cobertura sintática completa não comprova execução, alcance nem ausência de dados embutidos. A origem FDE da seleção é responsabilidade do chamador.

Os 11 intervalos de `build/research/image-resource-map.tsv` tiveram cobertura sintática completa, sem lacunas, registrada em `build/research/image-resource-bounded/coverage.tsv`. Os dois primeiros construtores haviam esgotado o timeout de 60 segundos do decompilador anterior; aquela execução foi interrompida e seu pseudocódigo não fundamenta este relatório. A API de disassembly foi conferida no `Base-src.zip` da instalação Ghidra.

Reprodução, com o projeto local já importado:

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/image-resource-map.tsv build/research/image-resource-bounded -max-cpu 1
```

## Origem dos campos de dimensão

Os getters anteriores confirmam largura em `Image+0x28` e altura em `Image+0x2c`. Há várias inicializações; nenhuma autoriza tratar o tamanho da grade OPT como dimensão original:

| Método metadata / início ELF | Escrita observada | Evidência ELF |
| --- | --- | --- |
| ctor 18372 / `0x1b8bdb8` | Ambos zero | `0x1b8bf0c`, store de 64 bits |
| ctor 18373 / `0x1b8bfac` | Resultados de duas chamadas virtuais no objeto recebido | `0x1b8c124–0x1b8c144`; slots `0x178` e `0x198`, ainda não resolvidos nesta análise |
| ctor 18374 / `0x1b8c230` | Dois valores preservados em registradores, após chamada auxiliar | `0x1b8c3e8`; rastreamento completo da assinatura pendente |
| ctor 18375 / `0x1b8c5dc` | Elementos 0 e 1 do resultado de `GetAreaSize` | call `0x1b8c75c`, stores `0x1b8c778/784` |
| ctor 18376 / `0x1b8cb04` | `240,240` | `0x1b8cc6c–0x1b8cc78` |
| ctor 18377 / `0x1b8cd28` | `240,240` | `0x1b8ce94–0x1b8ce9c` |
| ctor 18378 / `0x1b8cf50` | `240,240` | `0x1b8d0c4–0x1b8d0d0` |

Os últimos três valores são inicializações, não prova de dimensões finais. É necessário seguir o carregamento posterior antes de usá-los.

`Image.GetAreaSize` (18419, `0x1b8c828`, FDE 732 bytes) lê cabeçalhos do array fornecido: uma ramificação monta inteiros big-endian dos offsets de dados `16..19` e `20..23`, compatíveis com dimensões IHDR de PNG (`0x1b8c980–0x1b8c9e8`). Outra lê pares nos offsets `6..9`; outra percorre marcadores até `0xffc0`. A seleção depende de `GetFileFormat` e comparação de strings; esses literais ainda não foram resolvidos aqui. Portanto a observação não demonstra qual buffer alimenta os 18 chips.

O corpo de `LoadOptimize` examinado anteriormente não grava `Image+0x28/+0x2c`. Não apareceu uma regra `0=>256`, nem evidência de que o nome `optimize_624x640.inf` seja usado para construir dimensões.

## Cadeia de resolução confirmada parcialmente

`Seb.GetImage` (18972, `0x1baf470`) encaminha a busca ao `ResourceManager` quando seu campo `+0x88` está presente. `ResourceManager.GetImage` (18897, `0x1ba9a98`, FDE 156 bytes) consulta uma coleção em `+0x60`; se o resultado é nulo ou tem byte `+0xcc` diferente de zero, retorna o elemento do array `+0x10` indexado pelo ID recebido. O acesso tem verificação de limites; não se deve transformar IDs negativos de comandos SEB em índices.

`ResourceManager.LoadStart` (18865, `0x1ba6c58`, FDE 3516 bytes):

- Divide a entrada com `StringUtil.Split` (`0x1ba6f68`); quando há ao menos duas partes, converte a primeira via `JInteger.ParseInt` (`0x1ba6f98`) e preserva a segunda como nome (`0x1ba6fec–0x1ba6ff4`). O delimitador literal continua por resolver.
- Reorganiza coleções pelo ID explícito. Na ramificação de imagens, o array resultante é armazenado em `ResourceManager+0x10` (`0x1ba7638`). Isso sustenta o uso de IDs, não de ordem física de arquivos no ZIP.
- Para uma imagem não nula, obtém o nome via `GetFileName` (`0x1ba76cc`), calcula o arquivo de opção com `Image.GetOptionFile` (`0x1ba76f0`), verifica `JarInflater.Exist` (`0x1ba770c`) e lê `JarInflater.GetData` (`0x1ba773c`).
- Passa os bytes e o array de imagens para `Image.LoadOptimize` (18385), no call `0x1ba7754`. Essa ligação comprova que OPT e referências entre imagens pertencem ao carregamento do ResourceManager. O sufixo produzido por `GetOptionFile` ainda requer exame do método.
- Usa `IApplication.RunOnUiThread` em `0x1ba721c` e `0x1ba79d8`; a criação/carregamento delegado precisa ser seguido para fechar a origem das dimensões.

`LoadReady` (18863, `0x1ba64ec`) chama `StringUtil.LoadStringArray`, `Trim`, `Substring` e `Int32.Parse`; é consistente com os catálogos INF extraídos, mas os nomes dos catálogos e o delimitador precisam de confirmação por literais/callers. Nenhuma associação completa `TexId→img.inf→PNG` foi declarada apenas por correspondência de nomes.

## Próximo experimento delimitado

1. Resolver os destinos dos delegates de `LoadStart` e exportar somente seus intervalos FDE; seguir a chamada de construção/decodificação de Image e os stores finais em `+0x28/+0x2c`.
2. Inspecionar `Image.GetOptionFile` (18465, `0x1b91e48`) e `ResourceManager.GetFileName` (18867, `0x1ba7fcc`), identificando literais pela metadata/relocações.
3. Fechar a associação entre IDs dos catálogos extraídos e imagens do ResourceManager; validar todos os registros, incluindo IDs ausentes, variantes de idioma e conversão de extensões.
4. Só depois alterar o renderer: os 18 casos extrapolados, margens transparentes e tamanho original ainda não têm solução comprovada.

Não houve execução do jogo, mudança dos parsers/renderers nem implementação de comportamento aproximado neste ciclo.

## Continuação: closure UI, PNG e opções

Mais 11 intervalos foram exportados em `build/research/image-resource-followup/`, e 10 em `build/research/image-resource-final/`, todos com cobertura sintática completa nos respectivos `coverage.tsv`. Seleções reproduzíveis: `image-resource-followup.tsv` e `image-resource-final.tsv`. Mesmo comando anterior, substituindo seleção e diretório de saída; nenhum decompilador foi usado.

### Literais e caminho de opção

Referências de strings foram resolvidas por endereço ADRP/LDR explícito, relocação ELF até o slot de metadata e índice codificado v31, com limites verificados nas tabelas. Fórmula e layout: [Metadata.cs do Il2CppDumper, commit fixado](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Metadata.cs). Os arquivos locais `*.asm.literals.json` contêm apenas pares ADRP/LDR adjacentes conferíveis; não são propagação de registradores por fluxo arbitrário.

| Uso | Slot GOT ELF → slot metadata | Valor codificado / índice | Literal |
| --- | --- | --- | --- |
| GetOptionFile | `0x273de40→0x27ed6a8` | `0xa000082f` / 1047 | `.opt` |
| GetAreaSize | `0x273dd00→0x2801658` | `0xa0005827` / 11283 | `gif` |
| GetAreaSize | `0x273dd08→0x2803d48` | `0xa00061e3` / 12529 | `png` |
| GetAreaSize | `0x273dd10→0x2802568` | `0xa0005beb` / 11765 | `jpg` |
| closure LoadStart | `0x273e548→0x27ff3d8` | `0xa0004f87` / 10179 | `bin` |

`GetOptionFile` separa por vírgula (`0x1b91ea8`, caractere `0x2c`), usa a primeira parte, encontra o último ponto, remove a extensão e concatena `.opt` (`0x1b91ed8–0x1b91f0c`). `GetFileName` remove diretórios somente para prefixos `..\\` ou `../` (literais 1005/1003, slots GOT `0x273e3d0/3d8`), depois aplica `Trim`. Isso fecha a derivação `chip_00_0.png,bin,q+1 → chip_00_0.opt` na cadeia já documentada.

`GetAreaSize`: GIF lê largura/altura little-endian em offsets de dados 6/8; PNG lê inteiros big-endian em 16/20; JPEG procura marcador `0xffc0`, com altura e largura big-endian nos offsets do segmento. Esses ramos identificam dimensões da imagem codificada, não da área anterior à otimização.

### Caminho `bin` e dimensões depois do carregamento

Closure `<>c__DisplayClass60_0.<LoadStart>b__0`, método **18914**, ELF `0x1baa354`, FDE 5800 bytes:

1. A opção igual a `bin` ativa `w24=1` (`0x1baa63c–0x1baa654`).
2. Na ramificação sem cache de recurso, `0x1bab0f0` seleciona o caminho bin. Ele lê os bytes via `JarInflater.GetData` (`0x1bab124`), opcionalmente obtém a paleta, chama `OverwritePaletteData` (`0x1bab358`) e constrói `Image` com ctor18375 (`0x1bab390`). Sem paleta, `OverwritePaletteData` retorna o mesmo array (`0x1b8f90c→0x1b8f958`).
3. Esse construtor usa `GetAreaSize`, portanto inicializa `+0x28/+0x2c` com as dimensões PNG efetivamente armazenadas. O caminho alternativo com cache tem inicialização provisória e leitura posterior; não foi confundido com este caminho.
4. Em `Image.<Use>b__125_0` (18475, `0x1b92d88`, FDE 2212 bytes), `DecodeByteArray` recebe os bytes em `0x1b92e94`; se houver opção ainda não carregada, chama `LoadOptimize` em `0x1b92f00`. Depois, chamadas virtuais atualizam `+0x28/+0x2c` em `0x1b92f4c/64`.
5. Com origem de vtable `0x138` e stride 16 corroborados em `13-animation-formats.md`, os pares `classe+0x178/+0x198` são slots **4/6**. Texture (tipo6663, vtableStart83699) e Texture2D (6665,83708) contêm nesses slots `0x60019d85/0x60019d89`: métodos **52930 Texture.get_width** e **52932 Texture.get_height**. Assim, a atualização pós-OPT consulta o tamanho do Texture; não soma offsets das peças.
6. `DecodeByteArray` (18420, `0x1b8fe74`) cria Texture2D, chama `UnityUtil.LoadImage` (14654, `0x1c1ff20`) em `0x1b8ff90`, e retorna a textura. Nenhuma textura foi observada em runtime.

`Image.GetWidth/GetHeight` (18396/18395) apenas retornam `+0x28/+0x2c`, sem reconstrução OPT. A opção textual `WxH` na closure é separada por `x` e convertida em inteiros (`0x1baa924–0x1baa974`); `SetStandardResolution` (18461, `0x1b91cf0`) grava em **+0x70/+0x74**, não nos campos anteriores. Os 18 registros de `19_chip/img.inf` não possuem essa opção: todos usam `bin`, e somente IDs0/2 acrescentam `q+1/q-1`.

### Inventário correto dos 18 chips

O catálogo local `data/extracted/members/19_chip/0000_img.inf` define os IDs abaixo. Dimensões lidas diretamente do IHDR dos PNGs correspondentes no manifesto, não inferidas do nome nem do OPT:

| ID | PNG | IHDR largura×altura |
| --- | --- | --- |
| 0 | chip_00_0.png | 398×393 |
| 1 | chip_00_1.png | 624×381 |
| 2 | chip_00_2.png | 403×438 |
| 3 | chip_00_3.png | 624×385 |
| 4 | chip_01_1.png | 624×458 |
| 5 | chip_01_3.png | 624×464 |
| 6 | chip_02_1.png | 624×456 |
| 7 | chip_02_3.png | 624×459 |
| 8 | chip_03_1.png | 624×371 |
| 9 | chip_03_3.png | 624×445 |
| 10 | chip_04_1.png | 624×366 |
| 11 | chip_04_3.png | 624×373 |
| 12 | chip_05_1.png | 624×401 |
| 13 | chip_05_3.png | 624×377 |
| 14 | chip_06_1.png | 624×422 |
| 15 | chip_06_3.png | 624×427 |
| 16 | chip_07_1.png | 624×398 |
| 17 | chip_07_3.png | 624×396 |

`members/19_chip/0001_optimize_624x640.inf` contém literalmente `size = 624,640`, além de opções de processamento. Isso comprova a presença de uma configuração com esse tamanho; não demonstra que o runtime lê esse arquivo. Nenhum consumidor dele foi identificado neste ciclo. A dimensão original completa permanece separada das dimensões PNG confirmadas e do canvas ocupado de referência.

`UnityUtil.LoadImage` também foi exportado sem decompilador: FDE456 bytes, cobertura completa, `build/research/image-unityload/`. Faz seleção por versão (literal `2018.1.0`, índice1331), e invoca por nome `LoadImage` (5633), usando uma das duas formas: tipo `[UnityEngine.ImageConversionModule] UnityEngine.ImageConversion` (9389), com objeto Texture e bytes, ou método no Texture com bytes. Literais corroborados pelos slots `0x273cee0`, `0x2740150` e `0x2740138`. O wrapper não recebe OPT, INF, deslocamentos nem tamanho624×640; não contém resize explícito. A implementação interna Unity e o resultado da invocação não foram observados neste ciclo.

Próximo alvo: consumidor dos campos standard `+0x70/+0x74` e uso dos recortes SEB/OPT no desenho dos chips. A seleção do ResourceManager concreto usado pelo SEB de chips ainda precisa ser fechada por caller, embora o catálogo local e o caminho genérico de resolução por ID estejam documentados.
