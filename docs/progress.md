# Progresso

Atualização: 2026-10-25. **Prioridade: gameplay. M0/M1 parciais; ainda não há build jogável.**

## Concluído e verificável

- Divisão de textos de formulário em UTF-16 implementada com trim exato U+0000..U+0020 e campos vazios preservados (`87`). Windows/CTest14/14 passou; tradução e apresentação completas continuam pendentes.

- Dispatcher dos oito opcodes presentes na tabela ligado a progressão, stamina e callbacks de apresentação; teste integrado passou. Windows/CTest13/13 (`84`). UI e obtenção de estado de billing continuam pendentes.

- Efeitos dos opcodes1/2/4/15 e substituição de argumentos portados; sequência integrada com executor/espera testada. Condição original de subtração de cristais preservada com valor de sistema explícito. Windows/CTest12/12 passou (`83`).

- Executor de matriz e continuação por contagem implementado em `event_flow`, com host explícito para efeitos/resolução; Windows/CTest11/11 passou (`82`). Ainda sem ligação às telas, dispatcher completo ou loader do jogo.

- APK identificado e preservado; arquitetura Unity/IL2CPP e inventários documentados.
- Extração local de 30 contêineres/1.963 membros, incluindo 973 PNGs; leitores C++17 Jar/OPT/SEB e visualizador técnico implementados.
- Núcleo com contratos isolados de animação, componentes/filtros, payloads, dano, recompensas e estoque.
- Código dividido por responsabilidade e CMake modular. Git local em `main`, com baseline anterior à reorganização; conteúdo proprietário excluído.
- Reestruturação validada em Windows: build padrão e viewer, ambos com 10/10 testes CTest aprovados. Isso não valida fidelidade dinâmica nem integração do gameplay.

## Ativo

- Produtor de talks_ confirmado (`86`): BootForm usa Select/Join(TAB)/ToArray sobre TalkData. Split/trim nativos fechados em87; faltam tradução configurada, substituição de argumentos e flag de limpeza antes do modelo completo de apresentação.

- Reconstruir preparação dos formulários8/12 e ligar host aos registros reais (`84`); resolver origem de SYSSAVEI_YUGAMI e estado inicial do item de stamina. GetDelayId compara referências; as37 esperas extraídas não especificam hora (`81`). Produtor de SubForm+0x1d0 e outras rotas permanecem pendentes.
- Integrar contratos comprovados de combate, coleta e recompensas numa primeira simulação determinística.
- Resolver dependências de RNG, ordem de atualização e ciclo de entidades necessárias ao loop jogável, sem substituir lacunas por aproximações.

## Pendências e limites

- Não há Android disponível para comparação; nenhum jogo foi executado. Ainda há trabalho estático útil, portanto isso não bloqueia a pesquisa atual.
- Save completo, renderer de cena, entrada e menus integrados, áudio/fontes em execução, completude de conteúdo e funcionamento offline continuam pendentes.
- Builds Linux e avaliação de 3DS não realizados. Os testes Windows verificam contratos locais.
- REA Windows não suporta o perfil ARM ELF utilizado; a pesquisa nativa usa Ghidra headless direto.

## Próximos passos

1. Concluir o recorte de eventos citado acima e registrar contratos e incertezas.
2. Integrar o primeiro cenário determinístico de gameplay, mantendo regras, larguras numéricas e ordem de efeitos comprovadas.
3. Solicitar observação dinâmica quando ela se tornar o único avanço útil para a questão em análise.

## Navegação

- [Índice por capítulos](README.md): todos os relatórios e evidências.
- [Plano M0–M6](plan.md): critérios de conclusão e mapa dos módulos.
- [Changelog](../CHANGELOG.md): mudanças e validações.
- [Diário de pesquisa](research-log.md): histórico detalhado preservado.

M2–M6 não concluídos. A reorganização não altera o estado funcional do port.
