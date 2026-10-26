# Auditoria estática dos catálogos de recursos

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.

`tools/audit_resource_catalogs.py` verifica tamanho e SHA-256 dos 1.963 membros do manifesto antes de examinar `img.inf` e `seb.inf`. Preserva linhas, IDs explícitos, repetições e opções. O resultado enumera candidatos, **não implementa resolução runtime**, precedência de idioma ou fallback entre contêineres.

## Resultado observado

- 51 catálogos, 1.298 linhas não vazias; dois catálogos com BOM UTF-8.
- Um grupo de ID repetido: `30_chara/img.inf`, ID 2 nas linhas 3 e 4, ambas `m_chara02.png,bin`. Não eliminar repetições silenciosamente nem inferir política para valores conflitantes.
- Três linhas sem ID explícito: `03_lineup_layout/seb.inf` (`bg.seb`) e `34_resident/seb.inf` (`talk.seb`, `bustup.seb`). O auditor mantém ID nulo, sem atribuir índice de linha.
- 21 linhas não encontram o nome literal no mesmo contêiner, mas encontram candidato com extensão `.png`. Incluem nomes `.gif`; isso não prova que os bytes originais sejam GIF nem uma regra universal de substituição.
- Três linhas de `07_room` não têm candidatos locais: `img.inf` ID 0 `bg_long.png`; `seb.inf` IDs 4/5 `equip_frame01.seb` e `equip_frame02.seb`. Os três nomes existem em `09_shop`. Isso é coincidência verificável de nomes, não prova de fallback entre ResourceManagers, download obrigatório ou APK incompleto.
- Opções preservadas: `bin` (837), `u` (54), `q+1` (53), `a1` (7), `40x40` (2), `50x12` (2), `q-1` (2), `q-2/q-4/q-5` (uma cada). Contagem inclui variantes de idioma; não é contagem de recursos simultaneamente carregados.

Variantes `English.lproj/img.inf` e `zh-CN/img.inf` são analisadas separadamente. Candidatos na pasta do catálogo e na raiz são mantidos distintos. O auditor não transforma um `.seb` ausente em candidato `.png` e não busca outro contêiner automaticamente.

## Ligação com código e limites

`ResourceManager.LoadStart` (18865) chama `StringUtil.Split` em ELF `0x1ba6f68`; para resultado com ao menos duas partes, chama `JInteger.ParseInt` em `0x1ba6f98`, grava ID em `0x1ba6fb0` e armazena o nome da segunda parte em `0x1ba6fec` e seguintes. Export em `build/research/image-resource-bounded/ResourceManager_LoadStart_18865.asm` usa base Ghidra `+0x100000`. O literal separador desse caminho não foi resolvido nesta rodada.

`LoadReady` (18863) procura o caractere `0x09` antes de Substring/Int32.Parse em ELF `0x1ba6804–0x1ba6838`. Isso corrobora delimitador tab em um caminho nativo; não prova que toda regra do auditor reproduza StringUtil, BOM, precedência ou entradas sem ID. O parser é estritamente uma auditoria do corpus observado.

O caminho `.gif→.png`, a ligação `room→shop`, a precedência de idioma e a numeração implícita permanecem alvos estáticos úteis. Não dependem inicialmente de execução do APK. Após recuperar as regras, será possível compará-las com estes 1.298 registros sem inventar substituições.

Continuação: [26-resource-aliases.md](26-resource-aliases.md) comprovou a substituição textual `.gif→.png` e `.mld→.ogg`, condicionada ao seletor Config de valor 2/3/4, além de normalização e geração de candidatos por idioma. O auditor continua enumerando candidatos sem assumir esse seletor ativo. A associação room/shop permanece sem prova.

## Reprodução e validação

```powershell
python tools/audit_resource_catalogs.py data/extracted/manifest.json --out build/research/resource-catalogs.json
python -m unittest discover -s tests -p test_resource_catalogs.py
```

Três testes passaram: preservação de BOM/IDs repetidos/opções/linhas sem ID; rejeição de colunas desconhecidas; isolamento de contêiner/tipo e rejeição de bytes alterados. A suíte Python completa passou 23 testes. Duas avaliações do corpus produziram JSON idêntico, SHA-256 `3bde780a2ff358749e9e4b401b141facc076ccc1dbcab62d66326d6a67c38e85`; a auditoria independente de `data/full/manifest.json` (schema3, em vez do schema2 de `data/extracted`) produziu os mesmos bytes. Saída UTF-8/LF explícita evita diferenças de newline entre plataformas. Conteúdo proprietário permanece em diretórios locais ignorados.
