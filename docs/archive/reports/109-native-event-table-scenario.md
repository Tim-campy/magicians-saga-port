# Cenário nativo de eventos com tabela extraída

Data: 2026-10-25. Capítulo 4/M3. Integração de contratos dos relatórios76–84; não é execução do APK.

## O que agora pode ser executado

`event_trace` compõe executor de matriz, identificação por referência, fila de continuação, dispatcher e efeitos de progressão usando scripts fornecidos pela tabela local. `tools/run_event_scenario.py` interpreta Event.txt com o leitor de pesquisa existente e envia as matrizes por um protocolo técnico para o executável C++.

O catálogo conserva ordem e objetos separados por registro, inclusive matrizes null/vazias. O índice passado ao cenário é posição no catálogo, não o ID interno. Na continuação, o host procura a identidade da matriz e retorna o mesmo objeto do catálogo. Não foi implementado um loader completo do jogo ou a rota de diagnóstico para scripts externos ao catálogo.

Estado de dinheiro, cristais, modo de subtração, stamina, estado decodificado do item e personagem são argumentos obrigatórios. São fixtures fornecidas pelo pesquisador, não defaults de NewGame, saves reais ou alterações de itens/licenciamento.

As condições `terms` e o timing do registro não são avaliados: o cenário injeta explicitamente o script escolhido. Cada passagem solicitada representa uma entrada no trecho de processamento da fila, não frame, segundo ou simulação de SelectForm completa. Pedidos de fala/formulário são observados por callbacks; UI, bloqueios e encerramento dos formulários não são executados. Scripts que exigem estado de áreas/quests não fornecido ou opcodes desconhecidos falham explicitamente.

## Experimento com o registro10

Tabela inglesa local: `data/extracted/members/06_xls/0007_English.lproj_Event.txt`. SHA-256 `b625c2892879394ee150563771a09230b2176e29b7c670198fd7cb43bb5ca9fa`.

Estado inicial escolhido: dinheiro7, cristais11, modo10000, stamina3, estado do item0, personagem2. O registro de índice10 tem ID10. A matriz lida contém espera50, fala39, evento21 e créditos de cristais/dinheiro. Os conteúdos completos permanecem no arquivo local do usuário.

| Ponto | Dinheiro | Cristais | Fila | Espera restante |
| --- | --- | --- | --- | --- |
| Após execução inicial | 7 | 11 | 1 | 50 |
| Após passagem49 | 7 | 11 | 1 | 1 |
| Após passagem50 | 8007 | 1011 | 0 | — |

Stamina permanece3. Na retomada, o trace registra restauração do personagem2, pedido de fala39/push, pedido de evento21/push, seguido do estado final. A conferência automatizada local comparou esses três checkpoints e a ordem exata dos callbacks. Os efeitos monetários foram conferidos pelo estado final; o trace não registra cada store individual.

A repetição com `0030_Japanese.lproj_Event.txt` produziu trace idêntico. **As duas tabelas têm o mesmo hash**, portanto isso não constitui uma referência independente nem comparação de tradução. Nenhum resultado foi comparado com runtime Android.

## Reproduzir

```powershell
tools\build-desktop.cmd
python tools/run_event_scenario.py data/extracted/members/06_xls/0007_English.lproj_Event.txt --exe build/desktop/event_trace.exe --index 10 --passes 50 --money 7 --crystals 11 --crystal-mode 10000 --stamina 3 --item-state 0 --talk-character 2 --output build/research/event-scenario-en.json
```

O relatório JSON inclui hashes da tabela e do executável, estado inicial, registro selecionado, trace, código de saída e limites explícitos. O protocolo interno possui limites de tamanho de ferramenta, sem alegação de limites equivalentes do original. Nenhum conteúdo do APK é incorporado ao executável ou versionado.

## Validação e próximo passo

Windows/MSVC14.51: build sem warnings encontrados, **20/20 CTests passaram**; os dois cenários locais são verificações adicionais, não novos CTests. Log `build/research/event-trace-build.log`; resultados `event-scenario-en.json` e `event-scenario-ja.json`.

Confiança confirmed para composição nativa e resultados acima; ocorrência no jogo original não observada. Próximo: recuperar o avaliador das condições de EventData e seu caller para escolher eventos a partir de estado, reduzindo a necessidade de injeção manual. Depois ligar os pedidos aos formulários completos, mantendo suas guardas. Não chamar este cenário de primeiro loop jogável.
