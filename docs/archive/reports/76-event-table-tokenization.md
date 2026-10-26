# Tokenização e leitura dos eventos extraídos

Data: 2026-10-25. Evidência estática ARM64 e interpretação local dos dados; sem execução do APK.

## Contrato recuperado

EventData.Load usa StringArrayStream. Construtor36568 (0x1126cb4) chama Split com delimitador TAB, confirmado pelo GOT0x272feb8, e flags0/0/0: não remove campos vazios, não aplica trim nem escapes. GetString36573 retorna o próximo token; GetInt36575 encaminha a JInteger.ParseInt13380, que delega a System.Int32.Parse. Ambos incrementam o cursor antes de checar acesso/conversão. Não substituir campo vazio por zero.

GetIntDoubleArray36581 (0x112708c) lê contagem externa; negativa retorna null. Caso contrário aloca essa quantidade de linhas e, para cada linha, lê tamanho e os inteiros correspondentes. Tamanho interno negativo chega à alocação inválida, não significa linha nula. Zero externo significa array vazio, distinto de null. Exports em build/research/event-tokenizer e event-int.

O layout de EventData.Load é: id, nome, timing, matriz terms, matriz execs. Cada matriz é contagem externa seguida de tamanho/conteúdo por linha. Não há teste de fim da linha após a segunda matriz nesse corpo; tokens excedentes não devem ser rejeitados como se o original exigisse consumo total.

## Aplicação aos dados reais

Ferramenta tools/inspect_event_table.py lê o formato e preserva CR final no caminho sem normalização. Aceita o subconjunto observado de inteiros decimais com sinal e whitespace ASCII, com limites int32 explícitos. Não pretende reproduzir culturas, todos os erros de Int32.Parse, mutações de cursor após falha ou exceções de alocação do runtime.

Executada nos Event.txt inglês/japonês, cada um produziu228 registros,208 linhas de scripts de execução, timing0 em todos os registros e nenhuma matriz null. Todas as linhas foram consumidas sem tokens sobrando. O CR final dos tokens numéricos não impediu a leitura nesse subconjunto. Não atribuir significado aos opcodes apenas pelos seus valores.

Resultados locais: build/research/english-events-parsed.json e japanese-events-parsed.json, com hash de entrada e registros. Não redistribuir esses dados proprietários. Comandos:

```
python tools/inspect_event_table.py data/extracted/members/06_xls/0007_English.lproj_Event.txt --output build/research/english-events-parsed.json
python tools/inspect_event_table.py data/extracted/members/06_xls/0030_Japanese.lproj_Event.txt --output build/research/japanese-events-parsed.json
```

Verificações sintéticas adicionais passaram para nome vazio, matriz null, limites int32, tokens excedentes, truncamento, tamanho interno negativo e overflow de inteiro. São verificações da ferramenta/modelo, não comparação dinâmica contra o jogo. Sem C++ alterado, build nativo não repetido.

## Pendências

O delegate genérico que constrói cada EventData ainda precisa ser ligado ao corpo Load; seleção final de contêiner/idioma também permanece aberta. Não declarar loader nativo completo nem executor de scripts implementado. A ferramenta agora fornece registros estruturados para investigar opcodes e dados de gameplay com evidência concreta.
