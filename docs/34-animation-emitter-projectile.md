# Animação criada por emissores e projéteis

Data: 2026-10-25. Fecha os dois callers diretos de AddAnimation deixados abertos em `31-animation-producers.md`. Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`, ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Não houve execução do APK nem alteração de código C++.

## Evidência delimitada

Dois FDEs exportados sem decompilador por `ExportBoundedInstructions.java`: `EmitterSystem.Update37596`, VA `0x114ba28`, 2.132 bytes; `ProjectileSystem.Fire37886`, VA `0x1157230`, 480 bytes. Seleção em `build/research/animation-remaining-producers.tsv`; assembly, hashes e cobertura sintática integral em `build/research/animation-remaining-producers/`. Os exports somam `0x100000` aos VAs ELF usados abaixo.

Métodos chamados foram ligados por `research/local/il2cpp-code-map.json`; nomes de parâmetros foram lidos diretamente da tabela parameters da metadata v31 do APK. Cobertura de bytes e símbolos não equivale a alcance ou frequência em execução.

## ProjectileSystem.Fire: argumento anim controla a inclusão

A assinatura metadata contém `projectile`, `start`, `end`, `speed`, `parent`, **anim**, `rotate`, `maxHeightRate`. No prólogo, o booleano anim em w3 é preservado em w19 (`0x115726c`). O receptor da alteração é o argumento projectile preservado em x20, não uma entidade recém-criada dentro deste método.

Depois de configurar AddProjectile e, condicionalmente, AddModifyAnimation para rotação, o trecho `0x11573c8–0x11573dc` testa **bit0 de anim**:

- Desligado: pula AddAnimation e retorna pelo caminho normal.
- Ligado: passa **rate=1** em w1 e **backup=-1** em w2 para Entity.AddAnimation36765, com projectile em x0. Call site **0x11573dc**.

O flag rotate é preservado separadamente em w21 e testado antes (`0x1157380`); ele não substitui anim. Não habilitar animação automaticamente porque o projétil gira ou foi lançado. Não há teste local hasAnimation antes do AddAnimation; os efeitos de adicionar componente já presente pertencem ao contrato ECS, ainda a recuperar.

O método calcula parâmetros de trajetória antes desse trecho, com divisões/conversões float. Não foram reconstruídos como parte desta investigação; o resultado retornado não foi chamado de velocidade ou duração só pelo uso no registrador.

## EmitterSystem.Update: animação adicionada à partícula emitida

O call site **0x114be38** também passa **rate=1, backup=-1** (`0x114be2c/0x114be30`). Seu receptor é a referência retornada por **World.CreateParticle36980** em `0x114be24`; não é a entidade emissora original mantida em x20. A referência é conferida contra null em `0x114be28`.

No caminho que chega a esse call, CreateParticle recebe `resId=19`, `sebId=1` e `imgId` lido de BulletComponent.data+0x34 (`0x114bdf8–0x114be24`). Nomes dos argumentos vêm da metadata de CreateParticle. Posições vêm de Entity.get_position; velocidades vêm de Entity.get_speed e são divididas por float10 antes da criação. Isso descreve argumentos observados nesse caminho, não semântica completa do efeito visual.

### Guardas anteriores ao ramo

Para cada entidade enumerada, Entity.get_emitter36812 fornece o componente em `0x114bbb0`. Conservando os campos por offsets, sem atribuir nomes ainda não corroborados:

1. Lê inteiros emitter+0x18 e +0x1c; soma1 ao segundo, calcula quociente e resto por SDIV/MSUB, e grava o contador incrementado em+0x1c (`0x114bbbc–0x114bbcc`). Se o resto não é zero, vai à próxima entidade (`0x114bbd0`). Não assumir divisores positivos ou trocar por aritmética C++ indefinida em extremos.
2. Se emitter+0x20 !=-1 e quociente>esse valor, coloca a entidade em outra coleção do sistema e continua sem esse spawn (`0x114bbd4–0x114bc28`, com caminho de expansão em `0x114bc84`). O processamento posterior dessa coleção não foi tratado como remoção definitiva apenas pela inserção.
3. Exige emitter+0x10 ==0 (`0x114bc2c–0x114bc30`).
4. O ramo de CreateParticle/AddAnimation corresponde a emitter+0x14 ==0. Valores1 e2 têm outros caminhos e valores restantes seguem à próxima entidade (`0x114bc34–0x114bc48`). Não atribuir nomes de enum a0/1/2 sem evidência adicional.
5. Exige Entity.get_hasBullet36772 com bit0 ligado (`0x114bc54–0x114bc58`). Sem bullet, pula esse caminho.
6. Opcionalmente lê ModifyAnimationComponent+0x28 quando hasModifyAnimation está ligado; senão usa0 (`0x114bc64–0x114bd44`). Esse valor é usado depois na modificação da partícula, mas **não** condiciona AddAnimation.
7. Obtém BulletComponent.data e os componentes position/speed antes de CreateParticle; nulls têm saídas excepcionais. Após a criação não nula, AddAnimation(1,-1) não tem um flag anim adicional nesse trecho.

Assim, seria incorreto adicionar Animation à entidade emissora ao atualizar seu contador, ou tornar AddAnimation incondicional para todo tipo de emissão. Ele é incondicional apenas dentro do caminho de partícula selecionado e já validado pelos filtros anteriores.

## Resultado para a reconstrução

As sete referências diretas a AddAnimation no inventário `animation-producers/references.json` estão agora examinadas: cinco factories do relatório31 e estes dois callers. Todas passam1/-1 quando chegam à chamada. Isso sustenta esse par nesses **sites**, não defaults globais do componente nem imutabilidade de rate/backup. AddAnimation aceita argumentos explícitos, deserialização e stores diretos continuam possíveis.

O módulo `animation_transitions` permanece sem defaults globais, coerente com as condições de presença já recuperadas. Não foi implementado um emissor ou projétil parcial a partir desses poucos campos; ainda faltam trajetória, limites, dados e efeitos do registro ECS.

## Reprodução e próximos passos

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/animation-remaining-producers.tsv build/research/animation-remaining-producers -max-cpu 1
```

Validação: dois exports com cobertura integral, registradores/argumentos e condições acima confrontados com assembly, parâmetros corroborados na metadata. Não houve novo build ou teste de gameplay porque nenhum C++ mudou. Próximos alvos estáticos: significado dos campos de EmitterComponent e processamento da fila, contrato de componente duplicado, callers que decidem anim/rotate e resolução dos dados de partículas.
