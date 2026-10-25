# Encoding e terminações das tabelas

Data: 2026-10-25. Análise estática ARM64 e auditoria dos arquivos locais extraídos. Nenhum APK executado.

## UTF-8 confirmado

A chamada de JString.New13436 para0x1d07230 resolve Encoding.get_UTF85819. O método retorna um decoder UTF-8, criando-o quando necessário. Ambos os caminhos, com e sem BOM, usam esse provedor; prefixo EFBBBF é omitido na conversão. Export em build/research/data-encoding. A política exata para sequências inválidas continua pendente, mas os arquivos auditados não a exercitam.

## CRLF depende da versão

StringUtil.Split14535 compara separador com LF e chama UnityUtil.CheckUnityVersion14656 com literal6000.0.0 (GOT0x273d2e0). Quando retorna true, substitui CRLF (literal do GOT0x2739df0) por LF antes de separar. Não é uma flag arbitrária de configuração como permanecia em aberto em74.

CheckUnityVersion em0x1c1ea50 consulta/cacheia o resultado, obtém a versão do runtime Unity, extrai componentes numéricos, compara-os e retorna true para a versão corrente maior ou igual ao limiar. Os recursos desta cópia apontam Unity2022 (inventário anterior); nessa linha de versão o ramo de normalização6000 não seria usado. Ainda não foi capturado o valor retornado pelo runtime em execução, portanto preservar a condição em vez de removê-la do contrato geral.

Com normalização desativada, Split por LF preserva CR no fim de cada segmento. Como o trim está desativado na chamada de Data.Load, o parser de campos recebe esse CR e precisa ter sua própria semântica recuperada. Uma linha CRLF vazia resulta em segmento CR, não em string vazia, nesse caminho.

## Auditoria reproduzível

`python tools/audit_game_table_text.py data/extracted/members/06_xls --output build/research/game-table-text-audit.json`

O script verificou44 textos como UTF-8 estrito válido, nenhum com BOM. Encontrou5103 CR,5103 LF e5103 pares CRLF; não há CR isolado no conjunto auditado. Registra hashes, tamanhos e contagem de segmentos por arquivo sem copiar conteúdo proprietário para documentação. Essa auditoria não é emulação do decoder original nem teste dinâmico.

## Próximo passo

Resolver delegate de construção e tokenizador usado por EventData.Load, especialmente consumo de inteiros e scripts com CR final. Nenhum loader aproximado foi acrescentado. Sem C++ alterado, build não repetido; última validação nativa continua10/10 em70. O ciclo anterior foi progresso na ligação textual; este fechou encoding e a condição de normalização, além de medir os dados reais.
