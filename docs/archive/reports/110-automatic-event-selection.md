# Seleção automática de eventos e agregação de condições

Data: 2026-10-25. Capítulo4/M3. Análise estática ARM64; complementa o cenário109.

## Seleção do catálogo

AppData.ExecEvent34373 (0x110c580) percorre Data.eventData em ordem crescente. Pula entradas null, chama CheckTerm34365 para o índice e, se verdadeiro, chama ExecEvent34376 (0x110c668) com argumentos de contexto null. Só retorna antecipadamente se essa execução produz um objeto não nulo. Execuções com retorno null não interrompem a busca, mesmo que tenham alterado dinheiro, contador ou fila. Logo não implementar simplesmente “executar o primeiro evento elegível”.

O array é consultado novamente ao longo da busca, inclusive para obter o tamanho de cada iteração. Uma futura integração deve conservar a observação dessas alterações, sem capturar um snapshot de elegibilidade de todos os registros antes de executá-los.

## Guardas por registro

CheckTerm34365 (0x110b3e0) usa Lib.CheckLength36414, corpo compartilhado0x12d3b4c: array null, índice negativo, array vazio ou índice fora do comprimento retornam false. Depois rejeita registro null e EventData.isExecuted.

O getter isExecuted38304 (0x116f9f8) lê execNum_+0x20 e retorna **execNum_ > 0**, não “diferente de zero”. Contadores negativos não são rejeitados por essa guarda. Só então encaminha termsScript_+0x38 para o avaliador de matriz.

ExecEvent34376 continua distinto dessa seleção: seu corpo incrementa execNum_ em32 bits antes de verificar se há script, conforme72. Chamadas diretas a esse overload não passam automaticamente pelas condições. O cenário109 executa a matriz diretamente; não deve ser apresentado como execução do ciclo de seleção ou incremento do contador de EventData.

## Matriz de condições

CheckTerm34366 (0x110b534) chama IListExtension.IsNullOrEmpty33936, corpo compartilhado0x12cd9b8. O helper retorna true para null ou Count igual a zero, conforme despacho genérico de coleção. Esse resultado faz CheckTerm retornar **false**.

Para matriz não vazia, percorre condições em ordem e chama CheckTerm34368 (0x110b5f8). Para na primeira condição falsa; retorna true apenas quando todas passaram. Não trocar a matriz vazia por um AND matemático com resultado true. Linhas null não são ignoradas pelo agregador; são entregues ao avaliador unitário, cujas dependências/falhas ainda precisam ser modeladas.

Consequência para109: o registro10 possui terms vazio e não é elegível por essa busca automática. Seu teste foi uma injeção explícita do script, válida para testar continuação/efeitos, não uma prova da ocorrência automática desse evento.

## Comparadores e cobertura de dados

AppData.Compare34367 (0x110c1d4) usa operandos assinados de64 bits. Tabela de seis bytes em0x627b68 conferida diretamente: [0,5,8,11,14,17], relativa a0x110c1f8 com escala4.

| Código | Comparação |
| --- | --- |
| 0 | maior que |
| 1 | maior ou igual |
| 2 | menor ou igual |
| 3 | menor que |
| 4 | igual |
| 5 | diferente |
| Demais, incluindo negativos | false |

Esse helper não prova a semântica de todos os opcodes: alguns ramos do avaliador contêm comparações inline e outros chamam funções/coleções.

Na tabela inglesa de hash registrado em109 há228 registros:113 com terms vazio e115 com conditions não vazias. Frequências dos opcodes de condições:15:75,16:29,22:10,4:8,20:7,1:3,21:3,31:2,2:2,32:1,7:1,5:1,18:1,19:1. São14 opcodes distintos; não confundir com os oito opcodes de efeitos já implementados.

A tabela de salto unitária em0x627ae0 foi lida como32 entradas uint16. Destinos dos opcodes presentes:1→0x110b7e0,2→0x110b848,4→0x110b8c4,5→0x110b930,7→0x110b9b8,15→0x110bad4,16→0x110bb40,18→0x110bc98,19→0x110bce0,20→0x110bd28,21→0x110be00,22→0x110bf48,31→0x110c058,32→0x110c0ac. O corpo foi exportado, mas esses destinos ainda não equivalem a contratos completamente recuperados.

## Caller e limites

Busca B/BL imediato nos segmentos executáveis encontrou SelectForm.Update35965 chamando a busca34373 em0x11ec7c8; CheckTerm34365 tem caller direto em0x110c61c dentro da busca. No trecho de SelectForm, um byte estático+0x31 guarda a chamada; retorno não nulo salta a0x11ecb30, enquanto retorno null leva à limpeza desse byte em0x11ec7e8. Identidade do flag, seus produtores e guardas anteriores permanecem a conferir. Não inferir uma chamada por frame.

Evidências locais: `build/research/event-conditions/`, `event-condition-helpers/`, `event-conditions/direct-callers.json` e `delay-resume/M35965.asm`. Intervalos e hashes estão em coverage.tsv. Endereços excluem base Ghidra0x100000; ELF SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`.

Confiança confirmed para guardas, agregação, comparação e sequência da busca; significados completos dos14 predicados e seleção em runtime continuam unknown. Sem alteração C++ ou repetição de testes; build20/20 e cenário109 continuam sendo as últimas validações nativas. Próximo: recuperar os predicados mais usados15/16 e as dependências iniciais do avaliador, depois compor seleção por estado no executor técnico.
