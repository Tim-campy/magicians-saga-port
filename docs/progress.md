# Progresso

Atualização: 2026-10-25. **Prioridade: gameplay. M0/M1 parciais; ainda não há build jogável.**

## Concluído e verificável

- APK identificado e preservado; arquitetura Unity/IL2CPP e inventários documentados.
- Extração local de 30 contêineres/1.963 membros, incluindo 973 PNGs; leitores C++17 Jar/OPT/SEB e visualizador técnico implementados.
- Núcleo com contratos isolados de animação, componentes/filtros, payloads, dano, recompensas e estoque.
- Código dividido por responsabilidade e CMake modular. Git local em `main`, com baseline anterior à reorganização; conteúdo proprietário excluído.
- Reestruturação validada em Windows: build padrão e viewer, ambos com 10/10 testes CTest aprovados. Isso não valida fidelidade dinâmica nem integração do gameplay.

## Ativo

- Fechar execução e retomada de eventos: produtor da continuação SubForm+0x1d0, consumidores de hour_ e guardas de atualização (relatórios 77–79).
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
