# Registro de mudanças

Este índice descreve mudanças do projeto. Achados detalhados ficam no [índice de relatórios](docs/README.md) e no [diário de pesquisa](docs/research-log.md). Nenhuma entrada abaixo representa uma versão jogável.

## 2026-10-25 — Exceções dos parâmetros de texto

- Criado auditor limitado de callsites LSDA com hash, limites e encodings explícitos.
- Ligadas falhas decimal/hex aos handlers locais; ParseInt não recebe fallback global. [Relatório94](docs/94-text-parameter-exceptions.md).
- Dois testes Python aprovados e255 intervalos reais exportados; sem mudança C++.

## 2026-10-25 — Inventário de marcações e parser original

- Auditor de tabelas registra candidatos de marcação e suas localizações sem interpretar ou remover tags desconhecidas.
- Confirmados array de19 tags e tradução do texto inteiro antes do parser. [Relatório93](docs/93-presentation-markup-inventory.md).
- Sete testes Python aprovados; sem mudança C++.

## 2026-10-25 — Estado temporal nativo do texto

- Novo módulo text_reveal: comprimento dos blocos, velocidade por idioma, resets, avanço temporal, Flush e IsEnd.
- Preservados overflow, ordem de consultas e arredondamento; relógio e frames fornecidos pelo host.
- Windows/CTest17/17 aprovado. [Relatório92](docs/92-native-text-reveal.md). Parser e renderer continuam pendentes.

## 2026-10-25 — Temporização das falas

- Confirmada transformação de velocidade por idioma e compensação de chamadas no mesmo frame lógico.
- Documentados campos temporais e diferença Reset/SetText; comprimento depende dos blocos. [Relatório91](docs/91-text-layout-timing.md).
- Pesquisa estática, sem mudança C++ ou novos testes de produto.

## 2026-10-25 — Fluxo de páginas de fala

- Confirmados despacho do tipo0, distinção entre completar texto/avançar página e encerramento por contador.
- Documentado formato da continuação, sem aplicá-lo à fábrica inicial. [Relatório90](docs/90-talk-page-advance.md).
- Pesquisa estática; sem mudança de código ou nova execução de testes.

## 2026-10-25 — Dados de apresentação de gameplay

- Novo módulo presentation_forms prepara falas e mensagens usando os helpers UTF-16 existentes.
- Campos iniciais e ordem de parsing preservados; tradução, conversão numérica e catálogos exigem integração explícita.
- Windows/CTest16/16 aprovado, incluindo executor→fala→espera→evento. [Relatório89](docs/89-native-presentation-forms.md); interface visual ainda pendente.

## 2026-10-25 — Substituição nativa de argumentos

- Implementada substituição UTF-16 dos marcadores, com tradução/configuração explícitas e limpeza condicional do padrão de honorífico.
- Corrigida inversão documental de SanSanRemove em85 após releitura do branch; true habilita o caminho, false o pula. Não havia implementação nativa anterior dessa condição.
- Windows/CTest15/15 aprovado, incluindo integração substituir→split. [Relatório88](docs/88-native-presentation-arguments.md); tradução real e telas permanecem pendentes.

## 2026-10-25 — Split nativo da apresentação

- Regra de trim fechada: remove somente U+0000..U+0020, preservando espaços Unicode fora desse intervalo.
- Novo módulo `presentation_text` com splits TAB/& específicos dos formulários e distinção entre entrada vazia e campos vazios.
- Windows/CTest14/14 aprovado, incluindo varredura das fronteiras UTF-16. [Relatório87](docs/87-native-presentation-split.md); tradução completa continua pendente.

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
