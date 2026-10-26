# Fluxo de trabalho e Git

Leia [AGENTS.md](AGENTS.md), o [plano](docs/plan.md) e o [progresso atual](docs/progress.md). O repositório é local; publicar ou enviar a um remoto é uma ação separada.

## Ciclo de mudança

1. Identificar a pergunta e a evidência necessária; preservar as regras e limitações do original.
2. Fazer uma alteração pequena no módulo correspondente. Preferir um branch para trabalhos independentes: `git switch -c gameplay/descricao`.
3. Executar o build e os testes pertinentes. Para C++ no Windows: `tools\build-desktop.cmd`. Se alterar o viewer: `tools\build-viewer.cmd`.
4. Atualizar `CHANGELOG.md` e `docs/progress.md`. Para descobertas, atualizar o diário e criar um relatório numerado, incluindo evidência, confiança, limites e próximo passo. Adicionar o relatório ao capítulo correspondente em `docs/README.md`.
5. Revisar `git diff` e selecionar arquivos explicitamente com `git add <caminhos>`. Conferir `git diff --cached --stat`, `git diff --cached --check` e `git diff --cached` antes do commit.
6. Fazer um commit por mudança coerente, com mensagem que descreva o resultado. Não declarar fidelidade ao original apenas porque os testes locais passaram.

## Conteúdo local

APK, assets, saves, credenciais, ferramentas baixadas, bancos Ghidra, dumps, assembly e código decompilado devem permanecer fora do histórico. Usar `data/`, `research/local/`, `build/` e `.tools/` para os artefatos correspondentes, já ignorados. Não usar `git add -f` para contornar essas exclusões.

O histórico contém implementação própria, ferramentas e relatórios de pesquisa; não a distribuição do jogo. A configuração `.gitattributes` mantém fontes e Markdown em LF e scripts Windows em CRLF.

## Consulta

```powershell
git status --short
git log --oneline --decorate -10
git show --stat HEAD
git diff HEAD~1 HEAD
```

O commit inicial `787a81e` registra o estado anterior à reestruturação. Ele é um ponto de referência, não uma alegação de que o port estava concluído.
