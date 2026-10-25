# Interface de pausa e preparação do carregamento do mundo

Data: 2026-10-25. Continuação estática do relatório33. Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Nenhum APK executado, save acessado ou scheduler implementado.

## Evidência e limites

Quatro FDEs exportados por `ExportBoundedInstructions.java`, seleção `build/research/pause-hooks.tsv`, saída `build/research/pause-hooks/`: BasePauseSystem.OnEnabled37181 (28 bytes), OnDisabled37182 (28), ctor37183 (84) e World.Deserialize37014 (76). Todos têm cobertura sintática integral em `coverage.tsv`; não foi usado decompilador. Endereços abaixo são VAs ELF; exports somam `0x100000`.

Foram reutilizados PauseSystems37232 de `ecs-lifecycle/` e AnimationSystem.Pause37435 de `animation-transitions/`. Identidade de interface resolvida por relocação ELF e metadata v31, não pela aparência do loop.

## A coleção de pausa usa IPausable

O GOT **0x2731050** usado por SystemManager.PauseSystems aponta por relocação relativa a **0x27c7408**, cujo valor codificado é **0x200062f9**. A referência de tipo nativo é **12668**, correspondente ao byvalTypeIndex da metadata **4401, kairo.unity.ecs.IPausable**. A interface declara um único método, **Pause37206**, com um parâmetro.

Logo o método0 chamado em `0x113b1a4`, com argumento booleano preservado, é o contrato **IPausable.Pause**. A lista no manager+0x48 não é uma segunda lista genérica de Update.

AnimationSystem4450 tem interface offset **16** para tipo12668. Sua vtable começa em58921; a entrada58937 contém **0x60012477**, resolvendo a **37435 AnimationSystem.Pause** (`0x11420ac`). Isso fecha o despacho dessa implementação concreta sem assumir o comportamento das outras classes IPausable.

AnimationSystem.Pause compara enabled (instance.byte+0x18) com `(~argumento)&1`; se iguais retorna. Caso contrário escreve o novo valor antes de chamar slot5 para pausa ou slot4 para retomada (`0x11420ac–0x11420dc`). Conforme relatório33, os slots4/5 de AnimationSystem apontam para os hooks vazios BaseSystem.OnEnabled/OnDisabled. O caminho altera o flag; **não remove diretamente AnimationSystem da lista de Update**, cujo NeedsUpdate continua retornando true. O efeito real de uma pausa do jogo também depende do fluxo que chama World.Update e dos outros sistemas.

## BasePauseSystem provoca pausa global no manager

**OnEnabled37181**, `0x113b020`:

- Lê a referência em this+0x10 (`0x113b024`), usada como receiver do manager.
- Se nula, segue a saída excepcional (`0x113b028/0x113b038`).
- Passa **true** e faz tail call para SystemManager.PauseSystems37232 (`0x113b02c–0x113b034`).

**OnDisabled37182**, `0x113b324`, faz o mesmo com **false** (`0x113b328–0x113b338`). Esses hooks não contêm contador de nesting, inventário de outros motivos de pausa ou restauração de estado anterior. Isso não prova ausência desses controles no nível dos callers; não implementar um contador novo dentro desses hooks por convenção de outro motor.

O construtor37183 encaminha a BaseSystem..ctor37185 em `0x113b390`, após inicialização da classe referenciada. Ele não chama PauseSystems diretamente. A definição do estado inicial e a atribuição do manager devem ser seguidas pelo construtor base e SetWorld antes de portar a montagem do sistema inteiro.

A ordem importa: BaseSystem.set_enabled escreve o byte antes de entrar no hook; o hook encaminha ao manager, e PauseSystems registra isPause antes de despachar IPausable. A fase adicional OnPause dos sistemas gerais ocorre somente no caminho true, como descrito no relatório33. Não colapsar essas operações em uma única atribuição booleana.

## World.Deserialize: entidades antes de sistemas

World.Deserialize37014, `0x1135938`, tem somente76 bytes e segue esta ordem no caminho normal:

1. Lê World+0x20 e chama **EntityManager.Deserialize37095** (`0x1138ba8`) em **0x1135954**, usando o stream recebido.
2. Lê World+0x28 e chama **SystemManager.Deserialize37236** em **0x1135968**, com o mesmo stream.
3. Grava **1** em World.byte+0x38 em **0x1135970**, depois de ambas as chamadas retornarem.

Não há Destroy/RemoveAllSystems no wrapper. O relatório33 mostrou que SystemManager.Deserialize cria/adiciona sistemas antes de percorrer os callbacks de carregamento. Porém a ausência de limpeza nesses dois corpos **não prova duplicação numa execução**: o estado de entrada pode vir de World recém-criado, de limpeza anterior ou de efeitos em chamadas ainda não examinadas. Não acrescentar uma limpeza automática no port sem seguir os callers e EntityManager.Deserialize.

Essa escrita também dá proveniência ao byte+0x38 que AppData.InitWorld testa antes de criar sistemas (relatório32). O fato confirmado é: ele se torna1 após a desserialização bem-sucedida dessas duas camadas. Não foi atribuído a ele um nome original nem inferida validade geral do save.

## Reprodução e próximos alvos

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/pause-hooks.tsv build/research/pause-hooks -max-cpu 1
```

Validação:216 bytes em quatro FDEs integralmente exportados; ponteiro/índice de IPausable e entrada de vtable conferidos no ELF/metadata; tail calls e ordem de carregamento conferidos no assembly. Nenhum C++ modificado, portanto nenhum novo build ou teste comportamental foi alegado.

Próximos passos estáticos: BaseSystem constructor/SetWorld, callers de World.Deserialize e EntityManager.Deserialize, e seleção dos sistemas derivados de BasePauseSystem nas transições de menus. A lacuna de execução dinâmica não bloqueia esses passos.
