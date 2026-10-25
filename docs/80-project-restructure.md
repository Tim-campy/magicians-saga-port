# Reestruturação do projeto e início do versionamento

Data: 2026-10-25. Mudança de organização; nenhuma nova regra do jogo inferida.

## Problema e alteração

O CMake raiz reunia bibliotecas, ferramentas, dependências opcionais e testes. O núcleo tinha gameplay, ECS, animação e persistência no mesmo diretório; os79 relatórios não possuíam índice temático.

- Git iniciado em `main`; baseline `787a81e` preserva a estrutura anterior.
- 14 arquivos do núcleo distribuídos entre `gameplay`, `animation`, `ecs` e `persistence`, com includes ajustados.
- CMake dividido entre raiz, configuração comum, módulos, plataforma e testes. Nomes dos targets, C++17, avisos, hashes das dependências e caminhos de executáveis preservados.
- Índice dos80 relatórios organizado em capítulos do plano M0–M6, mantendo os caminhos históricos.
- Criados plano de módulos, changelog e fluxo de contribuição; progresso condensado para apresentar a situação atual. O diário e o Git preservam a evolução anterior.
- Exclusões Git e política de finais de linha explícitas. APK, assets extraídos, saídas de pesquisa e ferramentas locais permanecem fora do histórico.

## Validação

Os builds padrão e do visualizador passaram, cada um, nos10 testes CTest em Windows após a reorganização (`build/research/restructure-desktop.log` e `build/research/restructure-viewer.log`). O índice cobre os80 relatórios, sem duplicatas; links novos e exclusões Git foram revisados. Não houve nova execução gráfica ou comparação de pixels nesta mudança.

Os algoritmos foram preservados; mudanças nas unidades C++ limitam-se aos caminhos de include. Builds Linux e comparação com o jogo original continuam pendentes. Não há nova funcionalidade jogável entregue nesta reestruturação.
