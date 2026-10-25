# Progresso

Atualização: 2026-10-25. **Prioridade: gameplay. M0/M1 parciais; ainda não há build jogável.**

## Concluído e verificável

- Correção inicial da fonte portada no mesmo text_fit (103), com divisões sucessivas e flag após setter; Windows/CTest19/19. Fator fornecido explicitamente, sem pressupor valor de runtime.

- Controle de ajuste dos blocos portado em text_fit (102): redução iterativa, tolerância e ordem de consultas preservadas. Windows/CTest19/19. Medições reais e parser completo permanecem pendentes.

- Sequência decimal/hex/RGB portada em text_parameters (95), Windows/CTest18/18. Conversão real por cultura e layout permanecem pendentes.

- Contagem dos blocos e estado temporal de texto portados em text_reveal (92), Windows/CTest17/17. Parser de marcações, layout e desenho permanecem pendentes.

- Dados iniciais de falas/eventos implementados em presentation_forms (89); integração com executor testada, Windows/CTest16/16. UI e catálogos reais permanecem pendentes.

- Substituição de argumentos UTF-16 implementada e composta com split/trim (`88`); condição de SanSanRemove corrigida na documentação e testada. Windows/CTest15/15 passou. Tradutor real e telas completas ainda não implementados.

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

- Language.Init ligado a IApplication.Start; fábrica de falas cria Font padrão via TextLayout, com autoScale=!Japanese e localização habilitada (101). Seleção de recursos e código de idioma em99/100; faltam ajustes posteriores, escala efetiva e fallback interno Unity.

- Classe Java de FontTexture confirmada (98): Utility mede via Paint/SANS_SERIF e retorna duas medidas; leitor nativo rastreado. Falta origem efetiva de useFontTexture_ e fonte/escala das falas antes de escolher backend.

- Linhas dependem de medidas de trechos e duas rotas de fonte (96). Delegate resolvido em97/98; resolver useFontTexture_ e fonte/escala do boot antes de integrar layout real.

- Exceções decimal/hex resolvidas por LSDA (94); fallback0 pertence ao parser, não a ParseInt. Limpeza externa com relançamento fechada em95; conversão/cultura e integração continuam pendentes.

- Tags das quatro tabelas auditadas (93); texto inteiro passa por tradução antes do parser. Fechar parâmetros br/co/po e métricas para gerar blocos reais; sete testes Python aprovados.

- Temporização investigada (91): velocidade depende de IsShortWords; Frame combina relógio e TotalFrames. Contagem dos blocos portada em92; resolver parser e configuração efetiva antes de integrar.

- Avanço da fala mapeado (90): Frame→entrada→Flush ou próxima página; encerramento por contador e continuação pelo catálogo. Resolver produtores de nextIndex, temporização e fluxo separado do tipo4 antes de implementar.

- Produtor de talks_ confirmado (`86`): BootForm usa Select/Join(TAB)/ToArray sobre TalkData. Split/trim/substituição nativos em87/88; faltam tradução configurada, origem do flag e modelo completo de apresentação.

- Completar apresentação dos formulários8/12 e ligar host aos registros reais (`84`); resolver origem de SYSSAVEI_YUGAMI e estado inicial do item de stamina. GetDelayId compara referências; as37 esperas extraídas não especificam hora (`81`). Produtor de SubForm+0x1d0 e outras rotas permanecem pendentes.
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
