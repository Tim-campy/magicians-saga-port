# Registro de mudanças

Este índice descreve mudanças do projeto. Achados detalhados ficam no [índice de relatórios](docs/README.md) e no [diário de pesquisa](docs/research-log.md). Nenhuma entrada abaixo representa uma versão jogável.

## 2026-10-25 — Catálogo de falas ligado ao boot

- Resolvida cadeia BootForm→Select/Join(TAB)/ToArray→AppData.talks_, incluindo delegate e desvio fora do intervalo principal.
- Confirmados119 fragmentos em46 registros por idioma; tradução de argumentos permanece uma etapa distinta ainda parcialmente investigada.
- [Relatório86](docs/86-talk-catalog-producer.md); sem alteração nativa ou repetição de testes.

## 2026-10-25 — Preparação de textos e auditoria de apresentação

- Recuperados argumentos string/TAB, marcadores, preparação de fala e mensagens tipo&texto; diferenças de trim e limpeza condicional explicitadas.
- Nova ferramenta `inspect_presentation_table.py`:46 falas e24 mensagens por idioma, sem referências ausentes nas chamadas8/12 extraídas.
- Cinco testes Python aprovados; sem alteração C++ ou novo build. Evidência e lacunas no [relatório85](docs/85-presentation-text-preparation.md).

## 2026-10-25 — Stamina e dispatcher dos eventos conhecidos

- Mapeada decisão do limite10/20 a partir do estado de ITEM_STAMINA_UP; sem obtenção, alteração ou valor presumido desse estado.
- Implementada aritmética de stamina e dispatcher dos oito opcodes encontrados na tabela. Falas/formulários usam callbacks explícitos, ainda sem UI reconstruída.
- Preservadas duas leituras do limite, ordem criar/push/retornar e diferença entre opcode10 no executor e no dispatcher unitário.
- Windows/CTest13/13 aprovado; teste integra dinheiro, apresentação, espera e stamina. Evidência e limites no [relatório84](docs/84-stamina-and-event-dispatch.md).

## 2026-10-25 — Efeitos de progressão e argumentos

- Novo módulo `event_progression` com efeitos de dinheiro, cristais, áreas e missões e resolução de sentinelas sem alterar a linha original.
- Preservadas diferenças entre overrides de descoberta, wrap32/64 e condição de subtração de cristais selecionada pelo estado de sistema; nenhum valor inicial presumido.
- Integração de teste com executor/espera e argumentos capturados; Windows/CTest12/12 aprovado. Log local: `build/research/event-progression-build.log`.
- [Relatório83](docs/83-event-progression-effects.md) registra evidência e limites; falas, eventos visuais e stamina seguem pendentes.

## 2026-10-25 — Executor nativo e fila de eventos

- Novo módulo `event_flow`: matriz interrompida por opcode10, contexto capturado e retomada por passagem de contadores.
- Preservados identidade dos scripts, resultado do último despacho, cópia de argi, remoção antes de executar e limite inicial da fila. Efeitos e guardas externas exigem host explícito.
- Novo teste de contrato com sequências, mutações e falhas; build Windows padrão passou nos11 testes CTest (`build/research/event-flow-build.log`).
- [Relatório82](docs/82-native-event-flow.md) documenta evidência e limites; integração completa do gameplay continua pendente.

## 2026-10-25 — Identidade e origem das continuações de gameplay

- GetDelayId mapeado: associação por identidade de matriz, primeiro índice correspondente, fallback-1.
- Confirmado campo SubForm.delayEvent_; buscas limitadas não localizaram produtor e não demonstram ausência.
- Auditadas37 esperas da tabela extraída, todas sem hora explícita. Próxima implementação prioriza continuação por contagem, conservando guardas externas.
- Evidência e limites no [relatório81](docs/81-delay-origin-and-script-identity.md). Sem C++ alterado ou testes repetidos nesta rodada.

## 2026-10-25 — Reestruturação e versionamento

**Capítulos afetados:** infraestrutura de todos os marcos; prioridade funcional mantida em gameplay/M3.

- Iniciado Git local, branch `main`, com baseline `787a81e` anterior à reorganização.
- Separados14 arquivos de `src/core/` em `gameplay/`, `animation/`, `ecs/` e `persistence/`; includes atualizados, algoritmos preservados.
- CMake dividido em módulos, ferramentas desktop, testes e configuração comum. Preservados targets, caminhos dos executáveis e dependências opcionais fixadas por hash.
- Criado índice de80 relatórios em sete capítulos alinhados a M0–M6, sem mover os relatórios históricos.
- Criados [plano](docs/plan.md) e [fluxo Git](CONTRIBUTING.md); README atualizado e progresso condensado. Relatórios futuros devem entrar no índice e mudanças devem ser registradas aqui.
- Definidas exclusões de conteúdo local/proprietário e política de finais de linha.

**Validação:** `tools\build-desktop.cmd` e `tools\build-viewer.cmd` concluídos em Windows, cada configuração com10/10 testes CTest aprovados. Logs locais: `build/research/restructure-desktop.log` e `build/research/restructure-viewer.log`. Conferidos cobertura do índice, links novos, exclusões Git e ausência de mudanças nos algoritmos C++.

**Limites:** Linux e execução/comparação do jogo não testados. O visualizador foi compilado, sem nova validação visual nesta mudança. Não há remoto configurado ou publicação.

**Relatório:** [80 — Reestruturação](docs/80-project-restructure.md).

## 2026-10-25 — Baseline do trabalho anterior

**Commit:** `787a81e` — primeiro registro Git, sem reconstrução artificial de commits anteriores.

- Inventário do APK, investigação Unity/IL2CPP e79 relatórios estáticos existentes.
- Extração e leitores de assets, visualizador técnico e módulos C++17 isolados.
- Ferramentas de pesquisa, testes e documentação do progresso anterior.

Detalhamento por assunto nos [capítulos1–6](docs/README.md); limitações atuais em [progress.md](docs/progress.md).
