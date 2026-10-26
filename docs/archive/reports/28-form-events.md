# Eventos lógicos de formulário e contagem de passagens

Data: 2026-10-25. Continuação estática de `27-frame-policy.md`, sem executar o APK nem implementar uma simulação aproximada.

## Evidência

Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços abaixo são VAs ELF; exports Ghidra somam `0x100000`.

Fontes: `build/research/frame-policy-followup/M19675__execute.asm`, metadata v31 e mapa IL2CPP, mais `build/research/form-events/M19739_DoMethod.asm`. O novo export contém todos os 960 bytes do FDE, com cobertura sintática integral em `coverage.tsv`, sem decompiler. A tabela de salto e a constante float abaixo foram lidas dos segmentos file-backed do ELF; não são inferências a partir de nomes.

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/form-events.tsv build/research/form-events -max-cpu 1
```

## Código 1 é Update

Em `_execute`, o call indireto ELF `0x1be0ed0` usa manager como receptor, formulário em x1, código **1** em w2 e argumento nulo em x3. O par virtual é `classe+0x1f8`, isto é, slot12 com origem de vtable `0x138` e stride16.

Nas vtables de FormManagerBase (tipo2316, início24492) e form.FormManager (4274, início57317), slot12 contém `0x60009a37`, que resolve ao método **19739 FormManagerBase.DoMethod**, VA `0x1be48e4`. Isso liga o call ao dispatcher para essas classes, sem assumir o tipo de todo receptor possível.

DoMethod aceita códigos0..5; a tabela de bytes em ELF **0x68dd53** é `00 3c 0c 37 05 67`, aplicada como `0x1be4958 + 4*byte`. O resultado e os slots de FormBase são:

| Código | Destino do switch ELF | Slot no formulário | Método base identificado na metadata |
| --- | --- | ---: | --- |
| 0 | 0x1be4958 | 17 | Init,19611 |
| 1 | 0x1be4a48 | 19 | Update,19613 |
| 2 | 0x1be4988 | 18 | Draw,19612 |
| 3 | 0x1be4a34 | 20 | Resume,19614 |
| 4 | 0x1be496c | 25 | Suspend,19619 |
| 5 | 0x1be4af4 | 26 | Finish,19620 |

O ramo1 pode executar instrumentação antes do hook. Em `0x1be4b6c–0x1be4b78`, liga form.byte+0x78 se ainda zero; em `0x1be4b7c–0x1be4b8c`, chama o par `classe+0x268/+0x270` (slot19), preserva o booleano retornado e o devolve após limpeza. O método concreto depende do tipo do formulário; os nomes acima são os alvos base e os contratos virtuais, não uma lista de classes de gameplay já reconstruídas.

Isso também identifica os códigos3/4 usados nos caminhos de retomada/pausa descritos em `24`, sem dizer que todos os formulários os recebem em toda condição.

## Fórmula inteira anterior ao loop

No caminho com estado form+0x38 igual a2 (ou após a transição1→2), `_execute` calcula a quantidade de passagens. Se manager+0x90 difere do formulário atual, zera o acumulador inteiro **manager+0x98** (`0x1be0de4–0x1be0df0`). A identidade e os escritores do ponteiro+0x90 ainda não estão recuperados; não o chamar de “último frame” por suposição.

Se **form+0x50 == -1**, usa inicialmente `N=1` (`0x1be0df4–0x1be0dfc`, `0x1be0e58`). Senão, o trecho `0x1be0e00–0x1be0e50` corresponde às operações abaixo, conservando as duas leituras do campo e as duas consultas ao getter:

```text
Ra = leitura anterior de form+0x50 (w28)
Fa = primeira consulta GetTargetFps (w22)
Rb = segunda leitura de form+0x50 (w23)
A  = manager+0x98 (w29)
Fb = segunda consulta GetTargetFps (w0)
T  = trunc(1000 / Rb)
D  = trunc(1000 / Fb) - T + A
B  = max(trunc(Ra / Fa), 1)
Q  = trunc(D / T)
N  = Q + B
manager+0x98 = D - Q*T
```

`SDIV` opera sobre registradores de32 bits e trunca em direção a zero; `SUB/ADD/MSUB` preservam somente32 bits. A notação acima descreve o caminho sem divisores zero/overflow; não autoriza usar operações C++ com comportamento indefinido para casos extremos. Os blocos não contêm clamp universal de N para1: se N<1, o loop é pulado. O clamp mínimo1 aplica-se somente a B (`CMP/CSINC` em `0x1be0e34–0x1be0e3c`).

Não substituir esta expressão por um acumulador convencional `R/F` por parecer mais natural. Por exemplo, se as leituras forem estáveis Ra=Rb=60 e Fa=Fb=30, A=0, o trecho dá T=16, D=17, B=2, Q=1, N=3 e resto1. Isso é uma avaliação algébrica do trecho, **não** observação de três updates no jogo. Outros estados e o ramo temporal ainda afetam execução. A fórmula literal é a evidência a preservar até entender o papel e os produtores de cada campo.

## Correção por tempo decorrido

Somente quando manager+0x90 **é** o formulário atual e form.byte+0x54 está ligado, entra em `0x1be0f64`. Calcula `elapsed = CurrentTimeMillis() - manager+0x88`; se elapsed>999, mantém N anterior. Caso contrário, limita elapsed superiormente a `trunc(2000/GetTargetFps)` por comparação signed (`0x1be0f84–0x1be0fa0`). Não há clamp inferior explícito nesse trecho.

Para a taxa usada no float, lê form+0x50; se=-1, consulta GetTargetFps. Calcula em single precision:

```text
candidate_float = float(elapsed) / (1000.0f / float(rate)) + 0.1f
candidate_int = conversão para inteiro truncada (com ramo explícito para +infinito)
N = max(N, candidate_int)
```

O `s8` antes desconhecido vem de ELF **0x6278b8**, bytes `cd cc cc 3d`, float `0.10000000149011612`; carregado em `0x1be0d00–0x1be0d04` e somado em `0x1be0fd4`. Não há, nesta evidência, interpolação de sprites ou alteração de velocidade global. Faltam os escritores de timestamp+0x88 e a seleção desse caminho no fluxo completo.

## Condições de despacho e estado

O loop guarda o formulário em manager+0x28 e, após a primeira passagem, liga byte+0xd0. Um objeto estático consultado em `0x1be0ea0–0x1be0eb4` pode impedir o call DoMethod; sua identidade ainda não foi resolvida. Sem esse objeto, faz o evento1. Se o evento retorna true e o estado do formulário ainda é2, altera-o para3. Se o objeto estático está presente, segue diretamente para essa verificação de estado sem executar Update.

Depois compara GetTopForm com o formulário original; mudança interrompe o loop. Havendo outra passagem e estado3, restaura estado2. Estado4 liga um sinal acumulado que participa de uma chamada posterior de tratamento. Ao sair, limpa manager.byte+0xd0 e ponteiro+0x28. O caminho normal incrementa **form+0x48 uma única vez após o loop**, em `0x1be103c–0x1be1048`, inclusive quando N<1. Portanto esse contador não é uma contagem simples de chamadas ao Update.

O relatório não atribui nomes semânticos aos estados1/2/3/4 nem ao objeto estático. Também não cobre ainda toda a entrada de `_execute`, transições da pilha de formulários ou caminhos excepcionais. O alcance dessas condições não foi medido.

## Próxima investigação

1. Resolver os escritores de manager+0x88/+0x90/+0x98 e form+0x48/+0x50, além da identidade do objeto que suprime Update.
2. Resolver overrides concretos do slot19 nos formulários de boot/menu e no primeiro formulário de gameplay; o dispatcher agora está comprovado.
3. Ligar o avanço SEB ao Update efetivamente despachado, preservando distinção entre TotalFrames, contador do formulário, chamadas lógicas e eventos GUI.

Essa investigação ainda oferece passos estáticos claros. Não exige dispositivo Android para prosseguir e não prova fidelidade dinâmica.
