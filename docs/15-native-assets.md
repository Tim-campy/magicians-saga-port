# Primeiro leitor nativo de recursos — contêiner JarInflater

Data: 2026-10-25. Implementação C++17 independente de Android, Unity, SDL e
serviços. Esta etapa fornece uma biblioteca de leitura e uma ferramenta de
inventário; não implementa gameplay nem renderização.

## Evidência e contrato

O layout vem de `kairo.unity.io.JarInflater._load`, ARM64 VA `0x1bfb5d0`, e dos
30 contêineres reconstruídos byte a byte pelo extrator Python, conforme
[12-asset-extraction.md](12-asset-extraction.md). O código novo expressa
esse formato documentado; não contém assets, chave nem código decompilado.

`src/assets/jar_archive.hpp/.cpp` recebe ponteiro e comprimento, sem acesso a
arquivos ou APIs de plataforma. Retorna nomes UTF-8 como `std::string_view`,
tamanhos e offsets; o chamador deve conservar os bytes de entrada imutáveis
durante o uso das views. Payloads não são copiados. A lista de membros usa uma
única reserva de vetor.

Validações: inteiros BE32 com larguras fixas; tamanho total; índice; quantidade;
nomes; UTF-8; tabelas; flags zero; prefixos de comprimento; intervalos; membros
contíguos; consumo integral dos dados. Rejeita flags desconhecidas e arquivos
com lacunas em vez de presumir codecs ou formatos. Nomes duplicados, vazios ou
contendo controles são preservados, e não são usados como caminhos no disco.

Limites padrão explícitos: 128 MiB de entrada, 100.000 membros, 65.536 bytes
por nome. São limites de proteção da ferramenta, não limites medidos do jogo
ou orçamento de memória do 3DS. O chamador da biblioteca pode reduzi-los por
`JarLimits`. Falhas de formato geram `JarError`; falha de alocação continua
sendo exceção padrão C++.

`src/platform/desktop/asset_inspect.cpp` lê **um contêiner já decodificado** e
emite JSON UTF-8. Não aceita diretamente APK ou payload cifrado. Campos:

- `data_offset`: início absoluto da seção de dados.
- `members[].offset`: offset relativo nessa seção, apontando ao prefixo u32.
- `members[].payload_offset`: offset absoluto após o prefixo.
- `members[].name` e `size`: nome original e comprimento do payload.

Aspas, barras invertidas e todos os controles U+0000–U+001F são escapados.
Nomes UTF-8 inválidos são rejeitados. Erros vão para stderr: código 1 para
arquivo inválido/ilegível, 2 para uso incorreto. No Windows, `wmain` recebe
argumentos UTF-16 e os entrega a `std::filesystem::path`, sem conversão pela
página de códigos local. Outras plataformas usam `main` e caminhos nativos.

## Build e teste

Windows, na raiz:

```bat
tools\build-desktop.cmd
build\desktop\asset_inspect.exe data\extracted\textassets\00_hat.bin
```

O helper descobre Visual Studio via `vswhere`, ativa `vcvars64.bat` apenas no
processo filho, usa CMake/Ninja do Visual Studio e roda CTest. Não instala
dependências, altera PATH global ou requer permissões administrativas.

Linux com CMake e compilador C++17 disponíveis, **comandos ainda não executados**:

```sh
cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
./build/desktop/asset_inspect data/extracted/textassets/00_hat.bin
```

Não há dependências de compilação externas. CMake habilita `/W4 /permissive-`
em MSVC e `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion` em GCC/Clang.

## Validação executada

Windows x64: MSVC 19.51.36252.0, CMake 4.3.1-msvc1, Ninja 1.13.2,
Windows SDK 10.0.26100.0. Build Release passou sem diagnósticos de warning do
compilador. Log local: `build/research/native-build.log`.

CTest passou 1/1 executável de testes (`jar_archive_contract`). Ele contém
cenários sintéticos de contêiner vazio; payload vazio; nomes duplicados;
UTF-8 e controles; offsets/payload; cada truncamento do fixture; bytes extras;
inteiros extremos; limites; flags desconhecidas; prefixos divergentes;
lacunas/sobreposição; UTF-8 truncado, overlong, surrogate e acima de U+10FFFF;
entrada nula. Os checks continuam ativos em Release, sem depender de `assert`.

Comparação diferencial local: **30 contêineres, 1.963 membros coincidentes**
com `tools/extract_assets.py` e `data/extracted/manifest.json`. Foram comparados
ordem, nomes, tamanhos, offsets, bytes alcançados pelos offsets nativos e hashes
de payload. Resultado: `build/research/native-differential.json`.
Também passaram JSON com todos os 32 controles, aspas, barra invertida e
Unicode; saída vazia/código 1 para arquivo truncado; código 2 sem argumento.
Após adaptar a entrada Windows, o mesmo contêiner foi lido sob um diretório
com acento e nome contendo `á` e `😀` (caractere suplementar), lançado via
`subprocess` Python. Seu JSON foi idêntico ao do caminho ASCII, código zero.
Build e CTest passaram novamente. Isso valida caminhos UTF-16 nessa toolchain
MSVC; não constitui teste de todos os sistemas de arquivos ou de MinGW.

Para repetir a comparação principal em PowerShell:

```powershell
@'
import hashlib, importlib.util, json, pathlib, subprocess
root = pathlib.Path('.')
data = root / 'data/extracted'
m = json.loads((data / 'manifest.json').read_text('utf8'))
spec = importlib.util.spec_from_file_location('extractor', root / 'tools/extract_assets.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)
containers = members = 0
for f in m['files']:
    if f['transform'] != 'xor44':
        continue
    p = data / f['output']
    blob = p.read_bytes()
    native = json.loads(subprocess.check_output(['build/desktop/asset_inspect.exe', str(p)]))
    expected = extractor.unpack_container(blob)
    recorded = [r for r in m['members'] if r['container'] == f['output']]
    assert len(native['members']) == len(expected) == len(recorded)
    cursor = 0
    for n, (name, payload), r in zip(native['members'], expected, recorded):
        assert n['name'] == name == r['name']
        assert n['size'] == len(payload) == r['size']
        assert n['offset'] == cursor
        assert n['payload_offset'] == native['data_offset'] + cursor + 4
        assert blob[n['payload_offset']:n['payload_offset'] + n['size']] == payload
        assert hashlib.sha256(payload).hexdigest() == r['sha256']
        cursor += 4 + len(payload)
    containers += 1
    members += len(expected)
assert (containers, members) == (30, 1963)
print('MATCH:', containers, 'containers,', members, 'members')
'@ | python -
```

Essa comparação comprova o perfil dos arquivos fornecidos, não equivalência
de execução com o leitor Android. O comportamento de entradas malformadas é
deliberadamente defensivo; não foi atribuído ao original. Não houve teste
Linux, 3DS, benchmark ou medição de RAM. Próxima etapa: consumidor de recursos
que use esses offsets e os formatos SEB/INF comprovados, antes de renderer ou
regras especulativas de jogo.

## Apêndice — leitor nativo OPT regular

Incremento de 2026-10-25: `src/assets/opt_grid.hpp/.cpp` acrescenta o formato
regular comprovado por `tools/inspect_opt.py` e [18](18-optimized-textures.md),
`Image.LoadOptimize` (VA ELF `0x1b8d2dc`) e `GetOptimize` (`0x1b8e4c0`).
O prefixo `ff` seleciona variante ainda não suportada e é rejeitado
explicitamente, mesmo quando é o único byte do arquivo.

O parser preserva os dois passos unsigned do header, colunas/linhas como bytes
signed não negativos, ordem row-major das células, quantidade signed não
negativa por célula e sete shorts signed big-endian por peça. Os sete valores
permanecem em `OptPiece::values`, sem nomes de renderização. Header zero é
preservado como zero, não convertido em 256. Células vazias continuam vazias.
**Os passos e a grade não são dimensões de imagem nem de canvas.**

Uma primeira passagem valida toda a entrada e calcula a alocação exata. A
segunda preenche dois vetores contíguos: células com `first_piece/piece_count`
e peças com arrays de sete `int16_t`. Não há vetor por célula, I/O, filesystem,
SDL ou ligação ao renderer no core. A saída possui seus escalares e independe
do tempo de vida dos bytes de entrada. Conversão signed BE16 é aritmética,
sem depender de narrowing unsigned→signed específico do compilador.

Limites padrão: 32 MiB de entrada, 16.129 células e 2.048.383 peças, máximos
estruturais dos contadores signed-byte não negativos. Esses limites são
configuráveis por `OptLimits`, não medidas de RAM do jogo ou do 3DS. Flags
de renderização, seletores de imagem, crops e dimensões não são normalizados
ou interpretados pelo parser. Erros de formato geram `OptError`.

Build e testes continuam usando `tools\build-desktop.cmd`. O CLI ganhou:

```bat
build\desktop\asset_inspect.exe --opt caminho\arquivo.opt
```

JSON: `cell_width`, `cell_height`, `columns`, `rows`, `cells`; cada célula é uma
lista de peças, e cada peça é uma lista ordenada de sete inteiros. A forma
antiga `asset_inspect <decoded-container>` foi preservada. Caminhos Unicode
continuam passando pela entrada UTF-16 do Windows.

Validação Windows/MSVC executada:

- CTest **2/2** passou (`jar_archive_contract` e `opt_grid_contract`). Testes
  OPT cobrem valores signed extremos, passo 254/255, passos zero, células
  vazias/múltiplas, ordem row-major, ownership da saída, grade com eixo zero,
  todos os truncamentos de fixture, bytes extras, FF, contagens negativas,
  limites configuráveis, 127 peças/célula, grade 127×127 vazia e entrada nula.
- **286 arquivos / 2.559 células / 2.481 peças**: todos os escalares e a ordem
  do JSON nativo coincidiram com o parser Python. Tamanhos/hashes do manifesto
  foram conferidos; o encoder Python reconstruiu cada arquivo byte a byte.
- Os 30 JSONs de contêiner JarInflater continuaram byte a byte iguais ao
  resultado anterior, incluindo seus hashes de saída.
- FF, contagem negativa e bytes excedentes retornaram código 1 com stdout
  vazio; `--opt` sem arquivo retornou 2. Arquivo OPT com `á` e `😀` no nome
  produziu o mesmo JSON que o caminho ASCII.

Evidências locais: `build/research/native-opt-build.log` e
`build/research/native-opt/differential.json`. Não houve teste Linux ou 3DS,
renderização OPT, reconstrução de canvas ou comparação de execução Android.

Repetição diferencial principal em PowerShell, após a extração:

```powershell
@'
import hashlib, json, pathlib, subprocess, sys
sys.path.insert(0, 'tools')
import inspect_opt
root = pathlib.Path('data/extracted')
m = json.loads((root / 'manifest.json').read_text('utf8'))
files = cells = pieces = 0
for item in m['members']:
    if not item['name'].endswith('.opt'):
        continue
    path = root / item['output']
    data = path.read_bytes()
    assert len(data) == item['size']
    assert hashlib.sha256(data).hexdigest() == item['sha256']
    opt = inspect_opt.parse(data)
    expected = dict(cell_width=opt.cell_width, cell_height=opt.cell_height,
                    columns=opt.columns, rows=opt.rows,
                    cells=[[list(p) for p in c] for c in opt.cells])
    actual = json.loads(subprocess.check_output(
        ['build/desktop/asset_inspect.exe', '--opt', str(path)]))
    assert actual == expected
    assert inspect_opt.encode(opt) == data
    files += 1
    cells += len(opt.cells)
    pieces += sum(map(len, opt.cells))
assert (files, cells, pieces) == (286, 2559, 2481)
print('MATCH', files, cells, pieces)
'@ | python -
```

## Apêndice — leitor SEB nativo estrutural

Incremento de 2026-10-25: `src/assets/seb.hpp/.cpp` implementa apenas os
modos de disco 0/1 evidenciados por `Seb._load`, VA ELF `0x1baf5a0`,
`tools/inspect_seb.py` e [13](13-animation-formats.md). Não implementa a
compactação dos buffers runtime, animação, bounding boxes ou interpretação
de campos de apresentação.

O primeiro byte não negativo pertence ao contador BE16 de grupos (modo 0).
`ff` seleciona modo 1; outros primeiros bytes negativos são modos desconhecidos
e causam erro. Contadores de grupos/records e `header_value` são shorts signed
não negativos. O `legacy_value` de cada grupo do modo 0 é preservado como
short signed, inclusive negativo, e não substituído por uma contagem.

Modo 0 preserva dez shorts signed por registro e nenhum campo opcional.
Modo 1 preserva quatro shorts, `control` unsigned e os seguintes opcionais,
onde `d = values[1]`:

| Campo | Presença | Tipo preservado |
| --- | --- | --- |
| `extra_word` | -6 ≤ d ≤ -2, ou d ≥ 0 e control ≠ 0 | u32 big-endian |
| `pair` | d ≥ 0 ou d = -8 | Dois shorts signed BE |
| `tail_pair` | Sempre no modo 1 | Dois shorts signed BE |
| `flags` | d ≥ 0 | Byte unsigned, todos os bits |

Ausência é representada por `std::optional` e por `null` no JSON, diferente
de valor zero. Valores permanecem com nomes neutros. A saída contém dois
vetores contíguos de grupos e registros: cada grupo referencia um intervalo
por `first_record/record_count`. Não há vetor por grupo nem dependência de
plataforma no core. A entrada é validada integralmente antes da reserva dos
vetores, e a saída possui os valores independentemente dos bytes originais.

Limites padrão configuráveis: 32 MiB, 32.767 grupos e 1.000.000 de registros
totais. O limite total de registros é uma política de memória da ferramenta,
não um limite comprovado do jogo ou orçamento 3DS. Rejeita truncamentos,
contagens negativas, modo desconhecido e bytes finais não consumidos.

```bat
tools\build-desktop.cmd
build\desktop\asset_inspect.exe --seb caminho\arquivo.seb
```

O JSON contém `mode`, `header_value` e `groups`; cada grupo contém
`legacy_value` e `records`; cada registro contém `values`, `control`,
`extra_word`, `pair`, `tail_pair` e `flags`. Formas antigas de CLI Jar/OPT e
caminhos Unicode Windows continuam suportados.

Validação Windows/MSVC: build passou e CTest **3/3** passou. Testes SEB cobrem
dez combinações nos limites condicionais de modo 1, extremos signed BE16,
palavra u32 `0xffffffff`, flags/control 255, grupos vazios, ownership, todos
os truncamentos dos fixtures, bytes extras, modo desconhecido, contagens
negativas, limites customizados, 32.767 grupos vazios, 32.767 registros em um
grupo e ponteiro nulo. Não há fixtures proprietárias no teste.

Diferencial: **303 arquivos / 452 grupos / 2.686 registros** coincidiram
integralmente com `dataclasses.asdict(inspect_seb.parse(bytes))`, incluindo
todos os escalares, ordem e presença/ausência dos campos. Hashes/tamanhos do
manifesto foram conferidos e o encoder Python reconstruiu cada entrada.
Regressão: os 30 JSONs Jar e 286 JSONs OPT ficaram byte a byte idênticos aos
resultados anteriores. SEB sob nome `á-😀.seb` produziu saída idêntica à do
caminho ASCII. Erros estruturais retornaram 1 com stdout vazio; `--seb` sem
arquivo retornou 2.

Evidências: `build/research/native-seb-build.log` e
`build/research/native-seb/differential.json`. Não houve execução Linux/3DS
ou comparação com animação do jogo Android; essa validação é estrutural.

Repetição da comparação principal em PowerShell:

```powershell
@'
import dataclasses, hashlib, json, pathlib, subprocess, sys
sys.path.insert(0, 'tools')
import inspect_seb
root = pathlib.Path('data/extracted')
manifest = json.loads((root / 'manifest.json').read_text('utf8'))
files = groups = records = 0
for item in manifest['members']:
    if not item['name'].endswith('.seb'):
        continue
    path = root / item['output']
    data = path.read_bytes()
    assert len(data) == item['size']
    assert hashlib.sha256(data).hexdigest() == item['sha256']
    parsed = inspect_seb.parse(data)
    expected = json.loads(json.dumps(dataclasses.asdict(parsed)))
    actual = json.loads(subprocess.check_output(
        ['build/desktop/asset_inspect.exe', '--seb', str(path)]))
    assert actual == expected
    assert inspect_seb.encode(parsed) == data
    files += 1
    groups += len(parsed.groups)
    records += sum(len(g.records) for g in parsed.groups)
assert (files, groups, records) == (303, 452, 2686)
print('MATCH', files, groups, records)
'@ | python -
```
