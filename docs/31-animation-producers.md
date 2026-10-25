# Produtores dos componentes de animação

Data: 2026-10-25. Investigação estática do APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`, ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Complementa o módulo de transições do relatório30; não implementa factories incompletas de entidade.

## Método e cobertura

Foram exportados cinco FDEs em `build/research/animation-producers/` (AddAnimation, AddDirection, duas formas de AddSeb e AnimationSystem.Init) e cinco factories em `animation-spawn/` (MapChip, Nature, Human, Enemy, Effect). Todos com cobertura sintática integral em `coverage.tsv`, sem pseudocódigo. Seleções TSV homônimas. Endereços abaixo são VAs ELF; exports Ghidra adicionam `0x100000`.

Uma busca por BL/B imediato para as quatro operações Add encontrou21 sites em `animation-producers/references.json`; os sites das cinco factories foram confrontados com assembly. Isso não é inventário de todas as atribuições: indireções e stores inline podem existir. SHA do ELF foi reconferido contra mapa e FDE. Nomes de parâmetros foram lidos da tabela `parameters` v31 do APK, não inferidos de registradores.

## Add não fornece defaults universais

**Entity.AddAnimation36765**, ELF `0x112c488`: parâmetros `rate`, `backup`. Obtém componente de uma coleção reutilizável ou aloca outro. Em `0x112c594`, `STP w21,w20,[component,#0x10]` grava **ambos os argumentos**, inclusive ao reutilizar componente. Despacha o componente pelo slot virtual5 com índice **10** (`0x112c5a8–0x112c5b8`). A identidade completa do receptor e efeitos do registro ainda devem ser considerados antes de reconstruir ECS.

**Entity.AddDirection36802**, `0x112db20`: parâmetros `dir`, `locked`. Grava inteiro em+0x10 e `(locked &1)` em byte+0x14 (`0x112dbe8–0x112dbf0`), usando slot5 com índice **12**. A direção não é normalizada para0..3 nesse trecho.

**Entity.AddSeb36889**, `0x1131148`: grava argumentos `res/id/frame/layers` em offsets+0x10/+0x14/+0x18/+0x20 (`0x113121c–0x1131224`); passa índice **2** ao slot5. A sobrecarga36888, `0x11310b0`, recebe um único `layer`, aloca array de comprimento1, escreve esse inteiro e encaminha à outra (`0x1131100–0x113113c`). Um `layer=-1` continua sendo um elemento do array, não array vazio por conveniência.

Portanto o construtor vazio do componente não define o estado final. A lógica de spawn escreve valores explícitos depois de adquirir o objeto; pooling também não autoriza conservar valores antigos desses campos.

## Caminhos de criação conferidos

### Humanos — World.CreateHuman36972, 0x1133a14

O parâmetro inteiro `dir` entra em w1 e é preservado em w25 (`0x1133a5c`), enquanto coordenadas float usam registradores s. No caminho de criação que chega ao trecho:

- AddSeb em `0x1133d74`: `res=1`, `id=ADD32(dir,36)`, `frame=0`, `layer=-1`, preparados em `0x1133d64–0x1133d70`.
- AddAnimation em `0x1133d98`: **rate=1, backup=-1**, sem condição adicional entre os componentes nesse caminho.
- AddDirection em `0x1133da8`: `dir` original e `locked=false`.

Isso comprova o sentinela-1 para esse produtor, mas não para toda criação/deserialização de componente. A factory também cria outros componentes e depende de dados; não foi reduzida a esses três em uma implementação parcial.

### Natureza e chips

CreateMapChip36968 (`0x113368c`) passa `res`, `sebId`, `frame` e `sebLayer` explícitos a AddSeb em `0x1133724`. Depois lê o argumento booleano `animation` da pilha em `0x1133760` e testa bit0 em `0x113376c`: só quando ligado chama AddAnimation(1,-1) em `0x113377c`.

CreateNature36969 (`0x11337a0`) passa `res`, `sebId`, **frame0, layer-1** a AddSeb em `0x1133838`. Seu argumento `animation` é lido em `0x1133860` e testado em `0x1133870`; só quando ligado chama AddAnimation(1,-1) em `0x1133880`. A presença do componente não deve ser inventada para objetos estáticos.

### Inimigos — World.CreateEnemy36973, 0x1133fc0

No caminho aceito, obtém um registro de dados indexado pelo parâmetro enemyDataId. Para AddSeb (`0x1134290`):

- `res` vem do retorno da chamada em `0x1134198`, cujo argumento usa registro+0x50; o significado completo desse resolvedor fica pendente.
- `id` vem de registro+0x5c; se=-1, usa registro+0x30 deslocado dois bits à esquerda (`0x113419c–0x11341b0`). Não chamar esse fallback de direção sem seguir a tabela.
- `frame` vem de registro+0x60; se=-1, usa0 (`0x11341b4–0x11341c4`). **Inimigos não têm frame inicial invariavelmente zero.**
- `layer=-1`.

O parâmetro `dir`, preservado em w19 desde `0x1133ff0`, só produz AddDirection se diferente de-1 (`0x1134354–0x1134370`), com locked=false. Logo ausência de direção também é um caminho explícito do spawn.

AddAnimation(1,-1) só ocorre quando **BaseData.Check(1)** no registro retorna bit0 desligado (`0x1134374–0x1134398`; destino38070 `0x1169924`). O significado da flag1 não foi presumido a partir do efeito. O parâmetro `flag` da factory e os bits consultados anteriormente são outra coisa; não confundir com esse Check no registro de dados.

### Efeitos — World.CreateEffect36978

Chama AddSeb em `0x1134bb8` com res/ID/frame preservados em registradores e layer-1. O argumento `anim`, identificado na metadata, é lido da pilha em `0x1134c74`; bit0 controla AddAnimation(1,-1) em `0x1134c8c`. O argumento `loop` tem outro papel e não foi usado como substituto dessa guarda.

## Implicações para integração

- Manter presença opcional de animação/direção no módulo C++ é sustentado por factories reais, não apenas defesa contra estado inválido.
- O estado fornecido explicitamente em `EntityAnimation` continua correto: não adicionar defaults globais rate1/backup-1/frame0/direction0 para todas as entidades.
- Os5 produtores examinados usam rate1/backup-1 quando efetivamente chamam AddAnimation. Há outros callers identificados, **EmitterSystem.Update37596** e **ProjectileSystem.Fire37886**; seus argumentos ainda não foram recuperados. Não extrapolar os valores aos demais.
- Não perder array layers[-1], as variantes de frame inicial ou a distinção entre os flags do registro e da chamada.
- A escrita/reutilização de componentes vem antes do despacho ao ECS, que pode notificar grupos/sistemas. Um futuro factory portátil deve preservar esses efeitos e ordem, além dos valores.

## Reprodução e validação

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/animation-producers.tsv build/research/animation-producers -max-cpu 1
```

Repetir com `animation-spawn.tsv`/`animation-spawn`. Dez intervalos integralmente exportados e os argumentos/guards acima conferidos no assembly. Nenhum código do port foi alterado nesta etapa e nenhum teste dinâmico executado; não foi necessário repetir testes de código inalterado. Próximos alvos estáticos: resolvedor de recurso de Enemy, semântica de BaseData.Check(1), argumentos de Emitter/Projectile e efeitos de registro dos componentes.

Continuação: [34-animation-emitter-projectile.md](34-animation-emitter-projectile.md) fecha Emitter/Projectile: os sete sites diretos conhecidos usam1/-1, sob condições diferentes. Isso não altera o contrato de argumentos explícitos de AddAnimation.
