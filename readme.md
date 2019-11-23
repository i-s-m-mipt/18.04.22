# 18.04.22

В папке projects лежит публичный код (трех) проектов автономной интеллектуальной системы алгоритмической торговли ценными бумагами фондового рынка MOEX. Часть кода (обученные модели системы технического анализа и части системы фундаментального анализа на базе NLP) скрыта из вопросов конфиденциальности.

В качестве основного подкрепления используются библиотеки Boost 1.70.

Раздел <b>shared</b> -- общие компоненты 3-х проектов:

1) <b>logger</b> -- система логирования, используется по принципу идиомы RAII для трассировки и записи сообщений различного уровня в файловый сток. Реализована на базе Boost.Log. Активно используется по всему проекту совместно с механизмом исключений.

2) <b>object</b> -- обертка для оперирования данными и хранения их в системе общей памяти в строковом виде (см. далее).

3) <b>memory (view)</b> -- посредник между DLL-библиотекой actions и системой общей памяти, реализованной в основном проекте system. Необходим, т.к. при использовании DLL задействовано динамическое связывание. Реализован на базе паттерна шаблонный метод и идиомы NVI. Есть более производительная версия на базе паттерна CRTP и MixIn (в разработке).

4) <b>config</b> -- одинокий псевдоним для регулирования базового контейнера тегов, использующегося в общей памяти и других компонентах.

Проект <b>plugin</b> -- плагин системы QUIK для получения данных с серверов MOEX:

В терминале QUIK запускается lua-скрипт, который запускает DLL-плагин, написанный на C++. Плагин использует библиотеку qluacpp для взаимодействия с qlua API в QUIK. Т.о. запрашиваются данные графиков стоимости активов и стакана котировок. Далее данные записываются в защищенную разделяемую память в виде структуры данных очередь, откуда осуществляется их чтение основным проектом system.

1) <b>export.cpp</b> -- настройка main и stop callback для lua API в QUIK.

2) <b>script</b> -- lua-скрипт, запускающий C++ DLL плагин.

3) <b>market</b> -- Класс, отвечающий за управление сбором данных. Не использует возможности параллельного исполнения по причине запретов со стороны QUIK. 

4) <b>market/source</b> -- Класс, отвечающий за получение и сохранение данных по заданному активу и таймфрейму. Используются средства Boost.IPC, а также непосредственно задействуются средства библиотеки qluacpp.

5) <b>market/quotes</b> -- Класс, отвечающий за получение и сохранение данных по стакану котировок. Используются средства Boost.IPC, а также непосредственно задействуются средства библиотеки qluacpp. Также данные сохраняются во внешний сток для накопления истории и обучения комбинированных моделей технического анализа.

6) <b>window</b> -- сторонний код, предназначен для визуализации получаемых данных. Не используется по причине опоры на WinAPI.

7) <b>config</b> -- одинокий псевдоним для регулирования выбора qlua API.

Проект <b>action</b> -- набор реализуемых пользователем действий в DLL библиотеке:

Действия исполняются параллельно (асинхронно) и работают с ситемой общей памяти из system. При написании действий используется кодогенерация для создания шаблона действия, затем пользователь пишет свой код и пересобирает DLL библиотеку. За счет динамического связывания нет необходимости полностью останавливать и перекомпилировать весь проект, достаточно приостановить system, выгрузить DLL, внести изменения, загрузить новые действия и продолжить работу. Реализованные действия: система технического анализа, графический интерфейс для отображения результатов технического анализа, графический интерфейс для отображения постанализа стакана котировок, алгоритм разметки для исторических данных стоимости активов. Подобный механизм позволяет без труда расширять функциональность работающей системы, в разработке находится вариант с множественными DLL действий.

1) <b>action/shared/stream</b> -- сторонний код, дополнение для библиотеки SFML для более удобного форматирования текста.

2) <b>action/shared/python</b> -- инициализатор Python C/C++ API для запуска Python кода в действии технического анализа.

3) <b>action/shared/mapper</b> -- алгоритм разметки исторических данных для обучения моделей технического анализа. Есть корректируемые параметры.

4) <b>action/shared/market</b> -- Приемник биржевых данных. Подключается к разделяемой памяти, создаваемой в проекте plugin. Формирует данные для дальнейшей обработке в действии технического анализа. Также по причине частых сбоев на стороне QUIK есть резервный способ получения данных по стоимости активов с серверов ФИНАМ. См. Python-скрипт в этой директории. Он вызывается в одной из get-функций Market-а.

5) <b>action/UD0001</b> -- действие технического анализа. Получает подготовленные данные от Market (см. выше) и вызывает Python-скрипт с реализованными и обученными моделями машинного обучения для технического анализа. За реализацию данных моделей ответственен мой независимый коллега-исследователь. Результаты записываются по ключевым словам в систему общей памяти через посредника memory/view_base (см.выше).

6) <b>action/UD0003</b> -- отображение результатов технического анализа. Используется SFML. Есть альтернатива с использованием FLTK И  более приятным GUI, но из-за отсутствия средств современного C++ в FLTK код выглядит чрезвычано отвратительно. Данные запрашиваются из системы общей памяти по ключевым словам.

7) <b>action/UD0004</b> -- отображение результатов постобработки стакана. Аналогично предыдущему пункту, просто другие данные.

Проект <b>system</b> -- основной проект решения:

В system реализована система общей памяти на основе тегов (также данная система памяти является частью подсистемы NLP фундаментального анализа собственного производства), а также система загрузки действий из DLL и их параллельного асинхронного исполнения.

1) <b>system/system</b> -- управляющий класс верхнего уровня.

2) <b>system/memory</b> -- система общей памяти. Имеет централизованную и распределенную (мелкогранулярную) систему защиты на мьютексах. Узлы (Node) содержат наборы объектов, которые приписываются этим узлам из действий (см. выше). Некоторые компоненты memory скрыты по выше-изложенным причинам. View является производным классом от View_Base, действует на основе динамического полиморфизма и вышеуказанных паттернов на стороне System (на стороне action действует базовый класс view_base).

3) <b>system/action</b> -- класс-обертка одного действия. Обеспечивает параллельное асинхронное исполнение действия. При завершении используется механизм будущих результатов. За счет механизма исключений обеспечивает устойчивую работу в случае случайных сбоев, которые могут приходить со стороны QUIK, спасибо разработчикам из ArqaTech.
