# Seleção da fonte Unity por idioma

Data: 2026-10-25. Capítulo 4/M3. Pesquisa estática; não determina métricas observadas em uma partida.

## Origem e cache

Font._init17761 chama GetDefaultGUIFont17807 (0x1b78300) e GetGUIStyleId17816 (0x1b784ac), guardando o ID em+0x24. A implementação _getDefaultGUIFont17808 (0x1b7acf4) compara Language.GetLanguageCode14019 com o campo estático defaultGUIFontLanguage_+0x20. Se não mudou, retorna defaultGUIFont_+0x18; não repete a busca a cada medida.

Quando o código muda, invalida a fonte anterior, limpa o cache de fontes inválidas e zera defaultGUIFont_. Ao concluir, guarda o código original, atualiza registros de nomes inválidos e chama TextLayout.ClearCache19193 (0x1bbc8f4). As operações sobre nomes inválidos ainda não constituem um modelo completo do cache.

## Ordem de procura

Literais conferidos no ELF/metadados: GOT0x273d7a8=`Fonts/default`, 0x272fb60=`-`, 0x273d7a0=`-ja`.

Para código não nulo e não vazio, o método usa String.ToLower617, Trim623 e Split600 pelo caractere `-`. Não substituir essas operações dependentes de cultura por normalização ASCII sem comprovar equivalência. A procura chama Resources.Load53632 (0x23b8ca8) e verifica nulo pela igualdade de UnityEngine.Object53874 (0x23bd59c):

1. Se existem pelo menos dois componentes, tentar `Fonts/default-<componente0>-<componente1>`. Componentes posteriores não entram nesse caminho.
2. Se a fonte continua nula, tentar `Fonts/default-<componente0>`.
3. Se continua nula e Language.Japanese13990 é true, tentar `Fonts/default-ja`.
4. Se continua nula, tentar `Fonts/default`.

Chamadas relevantes: primeiro Load em0x1b7b098, segundo em0x1b7b16c, alternativa japonesa em0x1b7b250, base em0x1b7b300. O resultado é verificado/cast para UnityEngine.Font. Uma exceção de tipo não equivale a recurso ausente.

Não há neste método criação explícita de uma fonte do sistema quando todas as buscas retornam nulo. GetGUIStyleId procura um GUIStyle com a mesma fonte, ou cria GUIStyle e chama set_font com o argumento recebido. Se esse argumento é nulo, o comportamento posterior depende da implementação Unity; não foi comprovada equivalência com LegacyRuntime ou com SANS_SERIF do Android.

## Código de idioma

Language.GetLanguageCode14019 (0x1b18a94) retorna langPackCode_ estático+0xa0 quando não nulo/vazio. Caso contrário, consulta Japanese13990 e retorna `ja-JP` (GOT0x2739b00) ou `en-US` (GOT0x27349c0). Japanese compara Language.Get (0x1b1b208) com zero. O estado efetivo de Language.Get e o produtor de langPackCode_ ainda precisam ser ligados ao boot/configuração da partida.

## Cruzamento com os recursos do APK

ResourceManager, pathID13 de `assets/bin/Data/globalgamemanagers`, contém quatro chaves de fontes, cada uma com três entradas (Font e objetos associados). Chaves preservadas como armazenadas, em minúsculas:

| Chave | FileID externo | PathID do Font | Nome do objeto |
| --- | --- | --- | --- |
| fonts/default-en-switch | 1114 | 3 | default-en-switch |
| fonts/default-hi | 1115 | 3 | default-hi |
| fonts/default-ja | 1116 | 3 | default-ja |
| fonts/default-th | 1117 | 3 | default-th |

Não há chave `fonts/default`, `fonts/default-en` ou `fonts/default-en-us` nesse catálogo. Isso é ausência limitada ao ResourceManager inspecionado, não prova que a Unity não tenha qualquer outra rota de fallback. A seleção não contém uma regra que transforme `en-US` em `en-switch`; a existência desse arquivo não autoriza escolhê-lo para inglês.

O inventário possui também Font LegacyRuntime nos recursos padrão da Unity, pathID10102, fora das quatro chaves acima. As cinco fontes já foram extraídas e verificadas no relatório14. A análise atual liga nomes de busca a recursos disponíveis, sem afirmar qual fonte foi desenhada.

## Seleção de backend e limites

Os métodos _setSize17770 e _updateGUIFont17810 não apresentaram escrita em Font.useFontTexture_ nos trechos exportados. _updateGUIFont pode trocar o GUIStyle após consultar a fonte padrão e os caches; isso não é uma atribuição do flag. A distinção entre campos estáticos e de instância é essencial: há também requestFontOffset_ estático no offset0x48, diferente de useFontTexture_ de instância.

Confiança **confirmed** para chamadas, ordem e chaves inventariadas; **unknown** para fonte/backend efetivos nas falas, fallback interno da Unity e métricas. Não foi implementado um substituto aproximado de fonte.

## Evidência e próximo passo

Exportações locais: `build/research/font-selection-default/` (sete métodos), `font-language-selection/` (três métodos), TSVs e logs correspondentes. Cobertura sintática integral, sem execução. Endereços nos arquivos assembly incluem base0x100000; os VAs deste relatório já descontam essa base.

Inventário cruzado: `research/local/unity-assets.json`, mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`. Metadados e mapa IL2CPP já inventariados. Nenhuma alteração C++; último teste de produto permanece18/18, sem repetição neste ciclo.

Próximo: rastrear os produtores de langPackCode_ e a fonte entregue à fábrica de falas, mantendo a investigação de useFontTexture_ limitada às escritas efetivamente resolvidas. Ainda existe caminho estático útil.
