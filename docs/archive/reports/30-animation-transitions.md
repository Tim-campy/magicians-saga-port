# Transições de animação por entidade e primeiro módulo de regras

Data: 2026-10-25. Continuação do relatório29; mesmo APK `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`, ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Nenhuma execução do APK foi necessária nesta rodada.

## Evidência e escopo

Nove FDEs em `build/research/animation-transitions/` e dez em `animation-components/`, selecionados pelos TSVs homônimos e exportados sem decompilador. Todos têm cobertura sintática integral registrada em `coverage.tsv`. Os endereços deste texto são VAs ELF; exports Ghidra usam base adicional `0x100000`.

Foi implementado um módulo C++17 isolado, `src/core/animation_transitions.{hpp,cpp}`, com três operações comprovadas. Não implementa ECS completo, scheduler, seleção de máximos SEB, desenho, save ou atualização por tempo. O estado é fornecido explicitamente pelo chamador, sem inventar valores iniciais de spawn. O módulo não usa APIs Android, filesystem, relógio ou assets e não aloca em seus caminhos normais. Erros de componentes ausentes usam exceção própria, não ABI/objeto de exceção IL2CPP.

## Componentes e identidade dos predicados

Os getters Entity foram identificados pelo mapa IL2CPP:

| VA ELF | Método metadata | Papel |
| --- | --- | --- |
| 0x112c3f0 | 36763 get_animation | Componente com incremento+0x10 e retorno pendente+0x14 |
| 0x112c478 | 36764 get_hasAnimation | Presença usada por CutIn |
| 0x112da88 | 36800 get_direction | Componente com inteiro de direção+0x10 |
| 0x112db10 | 36801 get_hasDirection | Presença usada por CutIn |
| 0x112e4e4 | 36817 get_hasEnemy | Predicado de CutIn |
| 0x112f224 | 36837 get_hasHuman | Predicado comum |
| 0x1130544 | 36867 get_hasMonster | Predicado de Change/Restore |
| 0x1131018 | 36886 get_seb | Componente com recurso+0x10, ID+0x14, frame+0x18 |

A metadata nomeia os campos de AnimationComponent `rate`/`backup`, DirectionComponent `dir`/`locked` e SebComponent `res`/`id`/`frame`/`layers`. Os papéis dos offsets acima são sustentados pelas operações/callers, não apenas pela ordem dessa lista. O novo código usa nomes descritivos próprios (`clip`, `return_clip`), sem representar memória IL2CPP diretamente. `get_isPlayingCutInAnimation`37260, VA `0x113e398`, confirma que o valor+0x14 diferente de-1 indica restauração pendente.

## ChangeAnimation37436 — 0x11420e0

1. Se hasHuman **ou** hasMonster, obtém direction+0x10 e soma ao ID recebido (`0x11420fc–0x1142128`). Não há guarda hasDirection nesse caminho: componente ausente falha antes de mudar Seb.
2. Grava o ID em SebComponent+0x14 (`0x114213c`).
3. Se argumento booleano ligado, zera frame+0x18 (`0x1142140–0x1142154`); caso contrário conserva o frame.

Não altera o recurso nem o retorno pendente. A soma é ADD32 com wrap; não usar overflow signed C++.

## CutInAnimation37437 — 0x1142168

1. Sem hasAnimation, retorna sem exigir componente Seb (`0x1142184–0x1142188`).
2. Grava o **argumento explícito de retorno** em animation+0x14 (`0x114219c`). Não salva automaticamente o ID atualmente desenhado nem empilha estados.
3. Somente se hasDirection e (hasHuman **ou hasEnemy**), soma direção ao novo ID (`0x11421a8–0x11421e4`). A distinção **Enemy/Monster** é real nos destinos nativos.
4. Grava o ID em SebComponent+0x14 e zera frame (`0x11421f8–0x114220c`).

Chamadas CutIn consecutivas substituem o retorno pendente. Um Seb ausente provoca falha depois da escrita do retorno; o módulo preserva essa ordem, em vez de transformar a operação em transação atômica.

## RestoreAnimation37438 — 0x1141f40

1. Exige animation; se seu+0x14 é-1, retorna sem exigir Seb (`0x1141f58–0x1141f68`).
2. Copia esse valor para Seb+0x14 e zera frame (`0x1141f9c`, `0x1141fa8`).
3. Para hasHuman ou hasMonster, calcula grupo de quatro a partir do ID salvo e adiciona a direção corrente:

```text
adjusted = ID < 0 ? ADD32(ID,3) : ID
group = bits32(adjusted) AND 0xfffffffc
newID = ADD32(direction,group)
```

Evidência: `0x1142004–0x114201c`, ADD/CMP/CSEL/AND/ADD. Para negativos comuns, o ajuste corresponde a truncar a divisão por quatro em direção a zero, e não simplesmente mascarar o ID original. Os bits e a largura32 foram preservados também para extremos.

4. Só ao fim grava-1 em animation+0x14 (`0x1142034`). Se faltar direção, o ID copiado e frame zero permanecem e o retorno ainda não foi consumido. Não deduzir que todo ID de restauração já contém a direção correta: o código explicitamente recalcula esse caso.

O relatório29 mostrou que AnimationSystem.Update chama RestoreAnimation ao alcançar o máximo e, depois, aplica módulo ao frame com o máximo capturado antes da restauração. Essa integração ainda não está no módulo; não usar automaticamente o máximo do novo ID sem conferir o original.

## Outros métodos e serialização

NeedsUpdate37432 retorna true. Pause37435 só muda this.byte+0x18 se o inverso do argumento difere do valor atual; após store chama slot5 para pausa ou slot4 para retomada. Os alvos desses hooks não foram reconstruídos aqui. Serialize37439 e Deserialize37440 **do sistema** contêm apenas RET; isso não torna vazios os serializers dos componentes.

Os construtores de AnimationComponent, DirectionComponent e SebComponent examinados apenas encaminham ao construtor base; nenhum default de gameplay foi deduzido deles. Em particular, não inicializar backup=-1 no port só porque esse é o sentinela de Restore: a origem dos valores no spawn continua a recuperar.

AnimationComponent.Serialize37261 envia campo+0x10 a StreamUtil.WriteByte14470 e +0x14 a WriteShort14472. Deserialize37262 faz SXTB e SXTH ao gravar os respectivos campos32 em `0x113e47c/0x113e490`. SebComponent.Serialize37416 grava recurso com WriteByte, ID com WriteShort, frame com WriteByte e layers por StreamUtil2.WriteIntArrayAsByteArray36547. Isso confirma distinção entre representação em memória e em stream. Endianness, envelopamento e todos os consumidores ainda devem ser fechados antes de implementar um save; nenhum save foi criado ou alterado.

## Validação do módulo C++17

```powershell
tools\build-desktop.cmd
```

Windows x64/MSVC compilou o módulo e CTest passou **4/4** contratos (novo módulo e regressões Jar/OPT/SEB). O contrato de animação cobre:

- troca simples com/sem reset e conservação de recurso/rate/backup;
- diferença entre humano, monstro e inimigo na aplicação da direção;
- CutIn repetido substituindo o retorno, Restore consumindo-o uma vez;
- ausência de animation, Seb ou direção, inclusive alterações anteriores à falha;
- alinhamento de ID negativo e wrap32 em soma/restauração.

Valores esperados derivam dos caminhos e operações explicitados acima. Testes não executam o APK e não provam timing, cenas ou fidelidade visual. O módulo é uma reconstrução estática testada, não funcionalidade completa verificada contra o jogo. Linux e3DS não foram compilados nesta rodada. Arquitetura: structs pequenos com optional para presença de componentes; sem dependências gráficas. A camada de ECS futura poderá adaptá-los sem expor offsets binários a gameplay.

Próximos passos: produtores dos componentes no spawn, registro/ordem de AnimationSystem no ECS e integração do ciclo de frame com Restore. Ainda há investigação estática útil.
