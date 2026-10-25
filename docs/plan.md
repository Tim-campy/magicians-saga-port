# Plano por capítulos e marcos

O [índice de relatórios](README.md) organiza a evidência por assunto. A numeração dos relatórios continua cronológica; capítulos não significam marcos concluídos. Gameplay é a prioridade atual, sem dispensar suas dependências.

| Capítulo | Marco | Situação | Critério para concluir |
| --- | --- | --- | --- |
| 1. Reconhecimento e arquitetura | M0 | Parcial | Inventário, mapa de subsistemas, dependências e viabilidade sustentados por evidência; lacunas explicitadas. |
| 2. Assets e apresentação | M1 | Parcial | Extração reproduzível e apresentação verificada de sprites, fontes e áudio; leitores e visualizador isolados já existem. |
| 3. Boot, menus e entrada | M2 | Pesquisa estática | Navegação e entrada integradas, comparadas ao original em cenários repetíveis. |
| 4. Gameplay e simulação | M3 | Prioridade; módulos isolados | Primeiro ciclo jogável integrado, com combate, coleta, resultado e testes determinísticos; comparação comportamental pendente. |
| 5. Persistência e progressão | M4 | Pesquisa e codecs parciais | Save/load completo, progressão, economia e cobertura de conteúdo, com compatibilidade e efeitos verificados. |
| 6. Validação desktop | M5 | Infraestrutura parcial | Builds Windows/Linux, cenários de regressão e desempenho medidos; ainda não há release candidata. |
| 7. Viabilidade 3DS | M6 | Não iniciada | Medir RAM, VRAM, CPU, texturas, controles e telas antes de decidir viabilidade. |

## Sequência imediata de gameplay

1. Fechar os produtores e consumidores das continuações de eventos, inclusive espera por hora e fechamento de formulário.
2. Integrar somente contratos comprovados de combate, coleta, recompensas e progressão a uma simulação determinística.
3. Resolver RNG, ordem de atualização, entidades e entrada necessários ao primeiro ciclo jogável.
4. Comparar com o APK quando houver ambiente dinâmico disponível. Enquanto existir investigação estática útil, prosseguir sem exigir esse ambiente.

## Organização do código

| Diretório | Responsabilidade atual | Limite |
| --- | --- | --- |
| `src/core/gameplay/` | Dano, afinidade, fatores, recompensas e estoque | Não é combate completo nem campanha integrada. |
| `src/core/animation/` | Transições e payloads de animação | Sem renderização de cena. |
| `src/core/ecs/` | Slots de componentes e filtros | Sem World/scheduler integrado. |
| `src/core/persistence/` | Codec de payload de scripts | Não é leitor de save completo. |
| `src/assets/` | Leitores Jar, OPT e SEB | Sem conteúdo proprietário versionado. |
| `src/platform/desktop/` | Inspeção e visualizador opcional | SDL restrito à ferramenta desktop. |
| `src/platform/n3ds/` | Reserva arquitetural | Sem implementação ou medição de viabilidade. |
| `cmake/` | C++17 e avisos comuns por target | Dependências do viewer permanecem opcionais. |
| `tests/` | Contratos dos módulos e ferramentas Python | Não substituem o original como referência comportamental. |
| `tools/` | Extração, pesquisa e comandos de build | Caminhos mantidos para preservar comandos existentes. |

Cada módulo possui seu CMake. Os nomes de targets e caminhos dos executáveis foram preservados. O núcleo não depende de SDL, Android ou ferramentas de engenharia reversa.

## Registro e versionamento

- `CHANGELOG.md`: índice das mudanças de implementação, organização e validação.
- `docs/README.md`: índice temático de todos os relatórios numerados.
- `docs/progress.md`: situação atual e próximos passos.
- `docs/research-log.md`: diário de perguntas, métodos, achados e limitações.
- `DECOMPLOG.md`: acompanhamento técnico da reconstrução.

Relatórios existentes preservam seus caminhos e o contexto da data em que foram escritos. Referências históricas a arquivos em `src/core/` correspondem agora aos módulos acima; não reescrever evidência antiga como se a organização atual sempre tivesse existido.
