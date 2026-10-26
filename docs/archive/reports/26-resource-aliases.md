# Conversão de nomes e busca de recursos

Data: 2026-10-25. Investigação exclusivamente estática do mesmo APK/ELF identificado em `21-image-resource-mapping.md`. Nenhum arquivo do jogo foi modificado, nenhum nome foi corrigido no extrator e nenhum APK foi executado.

## Evidência exportada

`tools/ghidra/ExportBoundedInstructions.java` exportou sete intervalos FDE em `build/research/resource-aliases/`: GetInputStream13965, Exist13967, GetData13968, _search13970, GetResourceAsStream13971, CheckResourceAsStream13972, ConvertExtension13973. Todos têm cobertura sintática integral, registrada em `coverage.tsv`. A seleção está em `build/research/resource-aliases.tsv`.

Config.ConvertExtension13614 foi exportado em `build/research/frame-policy/M13614_ConvertExtension.asm` (292 bytes); StringUtil.Replace14532 e AssetReader.GetAccessFilesList16890 em `build/research/resource-alias-followup/` (184 e 2.684 bytes). Cada diretório contém relatório de cobertura. Esses exports usam endereço Ghidra = VA ELF + `0x100000`; abaixo são usados VAs ELF.

## Regra que explica os nomes GIF no catálogo

**Confirmado:** existe uma regra nativa que transforma `.gif` em `.png`. Ela está em **Config.ConvertExtension**, não depende de encontrar um PNG com nome semelhante.

Em `0x1bece94–0x1becee0`, Config.ConvertExtension lê um inteiro no início do armazenamento estático de Config (`classe+0xb8`, depois offset zero). Se for **2, 3 ou 4**, executa duas chamadas a StringUtil.Replace:

1. Call `0x1becf14`: entrada original, `.gif`, `.png`.
2. Tail call `0x1becf2c`: resultado anterior, `.mld`, `.ogg`.

Nos outros valores retorna a entrada sem essas substituições. A identidade/valor runtime desse seletor não é inferida apenas pelos nomes das constantes da metadata. A existência da regra é confirmada; sua seleção durante uma execução continua não observada.

O construtor estático Config13622 (`build/research/frame-policy-followup/M13622__cctor.asm`) inicializa esse seletor com **2**: carrega 16 bytes de ELF `0x625fb0` em `0x1bed2dc` e os grava no início do bloco estático em `0x1bed2f4`. Os quatro inteiros little-endian são `(2,0,100,30)`. Isso prova valor inicial compatível com a conversão, sem excluir atribuições posteriores. A classe é corroborada por GOT `0x272f5c0→0x27c5898`, referência `0x20004fef` → índice de tipo nativo10231 → metadata tipo1778 Config.

Literais conferidos por relocação ELF (GOT → slot metadata → referência v31), com limites das tabelas:

| GOT ELF | Slot metadata | Valor codificado | Índice literal | Conteúdo |
|---|---|---|---:|---|
| `0x273f480` | `0x27ed658` | `0xa000081b` | 1037 | `.gif` |
| `0x273dfc0` | `0x27ed6c0` | `0xa0000835` | 1050 | `.png` |
| `0x273f488` | `0x27ed690` | `0xa0000829` | 1044 | `.mld` |
| `0x273f490` | `0x27ed6a0` | `0xa000082d` | 1046 | `.ogg` |

**Não reduzir isso a troca de sufixo:** StringUtil.Replace chama String.IndexOf com posição inicial zero e modo numérico 4 em `0x1c19558–0x1c1956c`, reconstrói prefixo + substituição + restante, avança após a substituição e repete (`0x1c19578–0x1c195d0`). Portanto o código substitui ocorrências no texto, não apenas uma extensão final. Não implementar comportamento diferente usando simplesmente `Path.ChangeExtension`.

Essa regra oferece explicação estática para as 21 linhas com nomes GIF auditadas em `25-resource-catalog-audit.md`, mas não estabelece em quais telas/idiomas essas linhas serão usadas. A transformação `.mld→.ogg` é uma regra de nome; não demonstra codec, presença de áudio MLD ou forma de conversão de bytes.

## Camada JarInflater

JarInflater.ConvertExtension13973 (`0x1bfc300`) chama Config.ConvertExtension em `0x1bfc384`. Se o campo `this+0x20` é nulo, retorna esse resultado. Caso contrário, consulta `Path.GetExtension`, aplica `String.ToLower` à extensão e troca `.inf` ou `.ini` por `.txt` usando **Path.ChangeExtension** (`0x1bfc3b4–0x1bfc448`). Aqui a regra é efetivamente de extensão final, diferente do helper anterior.

Literais adicionais: GOT `0x273e340→0x27ed668`, índice1039 `.inf`; `0x273f910→0x27ed670`, índice1040 `.ini`; `0x2739c88→0x27ed6e8`, índice1055 `.txt`. O campo `+0x20` serve como prefixo concatenado aos caminhos em GetInputStream/GetData/Exist; esta análise não atribui a ele uma origem concreta para cada instância.

- `Exist` chama ConvertExtension (`0x1beaef4`). Com prefixo `+0x20`, concatena e chama CheckResourceAsStream; sem prefixo, chama `_search` e compara o resultado com nulo (`0x1beaf24–0x1beaf2c`).
- `_search` (13970, `0x1bfc468`) retorna nulo se nome ou dicionário `this+0x10` forem nulos. Aplica Trim (`0x1bfc4c4`), ConvertExtension (`0x1bfc4d0`), ToLower (`0x1bfc4dc`), e AssetReader.GetAccessFilesList (`0x1bfc508`). Percorre a lista na ordem, aplica ToLower a cada candidato e consulta o mesmo dicionário (`0x1bfc52c–0x1bfc574`). Ao primeiro sucesso retorna o item; sem sucesso retorna nulo. A consulta genérica em `0x14eb90c` não ganhou um nome de método por conjectura.
- `GetInputStream` tenta o caminho com prefixo; se o stream é nulo, usa `_search` (`0x1bfc064–0x1bfc0a8`). Após encontrar um registro, pode buscar `AssetReader.GetCustomResource` com campo `this+0x28` e nome do registro (`0x1bfc0ac–0x1bfc100`); senão lê os bytes internos. Não confundir esse recurso personalizado com uma busca arbitrária em todos os contêineres.
- `GetResourceAsStream` e `CheckResourceAsStream` tentam o nome e depois o nome concatenado com `.bytes`. Literal GOT `0x273f8f0→0x27ed5d0`, índice1020. Calls `AssetReader.Exist`/GetInputStream em `0x1bfbe78/98`, e segunda tentativa em `0x1bfbed4/ef4`.
- `GetData` tenta `Storage.Read` com caminho prefixado (`0x1beafe4–0x1beb010`), depois GetSize/GetInputStream. A ramificação GetSize == -1 termina com resultado nulo, podendo registrar mensagem (`0x1beb028–0x1beb164`). Não há nesse trecho consulta direta ao arquivo `09_shop`.

## O que continua aberto

AssetReader.GetAccessFilesList foi exportado integralmente. Seu corpo contém chamadas a Config.ConvertExtension, StringUtil.Split, Language.get_UseResourceLanguage (`0x1c90124`) e Language.GetFolder (`0x1c90224`). Isso confirma participação de idioma na geração de candidatos; a precedência completa e todos os prefixos ainda precisam ser reconstruídos por fluxo, não por ordem das chamadas isoladas.

Não foi demonstrado um vínculo `07_room→09_shop`. O fato de três nomes ausentes no primeiro existirem no segundo continua apenas uma observação do corpus. Os caminhos de override, os chamadores que constroem os ResourceManagers e o carregamento de formulários são próximos alvos estáticos concretos. Não há motivo para inventar um fallback global ou declarar download obrigatório.

## Reprodução

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/resource-aliases.tsv build/research/resource-aliases -max-cpu 1
```

Para o followup, substituir seleção/diretório por `resource-alias-followup.tsv` / `resource-alias-followup`. Config.ConvertExtension consta da seleção `frame-policy.tsv`. Os limites vêm de `build/research/il2cpp-unwind.json`; símbolos e destinos de calls vêm de `research/local/il2cpp-code-map.json`. A validação desta rodada é de cobertura de bytes, limites e correspondência com metadata; não é teste comportamental do jogo.
