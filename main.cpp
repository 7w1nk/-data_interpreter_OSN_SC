#include "pch.h"
#include "windows.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <ctime>
#include <locale>
#include <sstream>
#include <filesystem>
#include <queue>
#include "logparser.h"
#include <iomanip>


using namespace std;
using namespace std::filesystem;

bool findfile(wstring derictory_path, path& result) {
	queue <wstring> directores;
	directores.push(derictory_path);
	while (!directores.empty()) {
		wstring dirpath = directores.front();
		directores.pop();
		try {
			for (const directory_entry entry : directory_iterator(path(dirpath))) {
				try {
					if (is_directory(entry.status())) {
						directores.push(entry.path().wstring());
					}
					else {
						auto path = entry.path();
						if (path.filename() == "sw errors.simlog") {
							result = path;
							return true;
						}
					}
				}
				catch (filesystem_error) {
					continue;
				}
			}
		}
		catch (filesystem_error) {
			continue;
		}
	}
	return false;
}

void error_occurred(int num, long stroke, wofstream& write_query, wstring errors_create, bool& is_errors_created) {													 //	Функция вывода ошибок при проверке данных
	map <int, wstring> error_description = {
	{-1,L"нет элементов"},
	{0,L"буквы в строке"},
	{1,L"недопустимый тип пакета"},
	{2,L"недопустимая причина ошибки"},
	{3,L"недопустимый тип исходеного символа SpaceWire"},
	{4,L"недопусимый тип механизма"},
	{5,L"недопустимое событие"} };
	if (!is_errors_created) {
		write_query << errors_create;
		is_errors_created = true;
	}
	write_query << L"INSERT INTO errors VALUES(" + to_wstring(stroke) + L",'" + error_description[num] + L"');" << endl;
}

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

wstring parsLine(const wstring& str) {
	wstring resLine;
	for (unsigned i = 0; i < str.size(); i++) {
		if (str[i] == '"') {
			continue;
		}
		else {
			resLine.push_back(str[i]);
		}
	}
	return resLine;
}


int main(int argc, char* argv[]) {

	setlocale(LC_ALL, "ru");

	map <wstring, wstring> type_id = {													//	Массив Типов пакета
	{L"0",L"Неверный пактет"},
	{L"1",L"Команда управления"},
	{L"2",L"Срочное сообщение"},
	{L"3",L"Обычное сообщение"},
	{L"4",L"Пакет запроса установки соединения"},
	{L"5",L"Пакет управления соединения"},
	{L"6",L"Синхронизация кредитов"},
	{L"7",L"Срочное сообщение транспортного соединения"},
	{L"8",L"Обычное сообщение транспортного соединения"},
	{L"9",L"Подтверждение для сообщения ТС СТП-ИСС"},
	{L"10",L"Подтверждение для сообщения СТП-ИСС"},
	{L"11",L"Прерывание"},
	{L"12",L"Подтверждение прерывания "},
	{L"13",L"Тайм-код"},
	{L"14",L"Обслуживающий пакет"},
	{L"20",L"Подтверждение установки соединения"},
	{L"21",L"Соединение установлено"},
	{L"22",L"Запрос закрытия соединения"},
	{L"23",L"Подтверждение закрытия соединения"},
	{L"24",L"Запрос установки соединения"},
	{L"25",L"Соединение закрыто"},
	{L"101",L"Команда записи"},
	{L"102",L"Команда чтения"},
	{L"103",L"Команда модифицированной чтения-записи"},
	{L"104",L"Пакет ответа"},
	{L"255",L"Неизвестный тип"} };

	map <wstring, wstring> reason = {														//	Массив причины ошибки
		{L"1",L"Искажение бита"},
		{L"4",L"Следствие предыдущей ошибки"},
		{L"5",L"Искажение бита + Следствие предыдущей ошибки"} };

	map <wstring, wstring> type_SpaceWire_id = {											//	Массив Типа исходного символа SpaceWire
		{L"0",L"Разрыв соединений в канале"},
		{L"1",L"ESC"},
		{L"2",L"Тайм-код"},
		{L"3",L"Прерывание"},
		{L"4",L"Подтверждение прерывания"},
		{L"5",L"Дополнительный код"},
		{L"6",L"NULL код"},
		{L"7",L"FCT"},
		{L"8",L"N-Char"},
		{L"9",L"EOP"},
		 {L"10",L"EEP"} };

	enum event {
		DEVICE_FAILURE = 0,
		PORT_FAILURE,
		UNIT_SWITCH,
		CHANNEL_ERROR
	};
	enum mechanism_type {
		PER = 0,
		BER
	};
	enum error_reason {
		BIT_DISTORTION = 1,
		CONSEQUENCE_PREVIOUS_ERROR = 4,
		BIT_DISTORTION_AND_CONSEQUENCE_PREVIOUS_ERROR = 5
	};


	wstring main_create = L"CREATE TABLE event (\n"											//Структура данных
		"  id INTEGER PRIMARY KEY,\n"
		"  `событие` text NOT NULL\n"
		");\n"
		"INSERT INTO event VALUES\n"
		"(0, 'Отказ устройства'),\n"
		"(1, 'Отказ порта'),\n"
		"(2, 'Переключение комплекта'),\n"
		"(3, 'Ошибка в канале');\n"
		"CREATE TABLE main (\n"
		"  id INTEGER PRIMARY KEY ASC,\n"
		"  `время записи` double NOT NULL,\n"
		"  `id в симуляторе` integer NOT NULL,\n"
		"  `id в vipe` integer NOT NULL,\n"
		"  `id комплекта` integer NOT NULL,\n"
		"  event_id integer NOT NULL,\n"
		"  `порт` integer DEFAULT NULL,\n"
		"  `id нового комплекта` integer DEFAULT NULL,\n"
		"  FOREIGN KEY (event_id) REFERENCES event (id)\n"
		");\n";
	wstring per_create = L"CREATE TABLE per (\n"
		"  main_id INTEGER NOT NULL,\n"
		"  `один из скольки пакетов искажается` integer NOT NULL,\n"
		"  `номер искаженного пакета` integer NOT NULL,\n"
		"  `номер искаженного байта пакета` integer NOT NULL,\n"
		"  `номер искаженного бита` integer NOT NULL,\n"
		"  `тип пакета` text NOT NULL,\n"
		"  `длинна пакета` integer NOT NULL,\n"
		"  `средняя длинна пакетов данного типа по трафикам` integer NOT NULL,\n"
		"  FOREIGN KEY(main_id) REFERENCES main(id)\n"
		");";
	wstring ber_create = L"CREATE TABLE ber (\n"
		"  main_id INTEGER NOT NULL,\n"
		"  `один из скольки битов искажается` integer NOT NULL,\n"
		"  `причина ошибки` text NOT NULL,\n"
		"  `номер искаженного символа SpaceWire` integer DEFAULT NULL,\n"
		"  `номер искаженного бита` integer DEFAULT NULL,\n"
		"  `тип исходного символа SpaceWire` text DEFAULT NULL,\n"
		"  FOREIGN KEY(main_id) REFERENCES main(id)\n"
		");";
	wstring ch_err_create = L"CREATE TABLE type (\n"
		"  id INTEGER PRIMARY KEY,\n"
		"  `тип механизма искажения` text NOT NULL\n"
		");\n"
		"INSERT INTO type VALUES\n"
		"(0, 'per'),\n"
		"(1, 'ber');\n"
		"CREATE TABLE channel_error (\n"
		"  main_id integer NOT NULL,\n"
		"  `Начальное представление пакета SpaceWire` text NOT NULL,\n"
		"  `Полученный в ходе искажений пакет SpaceWire` text NOT NULL,\n"
		"  type_id integer NOT NULL,\n"
		"  `id пакета` integer DEFAULT NULL,\n"
		"  FOREIGN KEY(main_id) REFERENCES main(id),\n"
		"  FOREIGN KEY(type_id) REFERENCES type(id)\n"
		");";
	wstring errors_create = L"CREATE TABLE errors (\n"
		"  `номер строки` integer NOT NULL,\n"
		"  `ошибка` text NOT NULL\n"
		");\n";
	bool is_main_created = false,
		is_per_created = false,
		is_ber_created = false,
		is_ch_err_created = false,
		is_errors_created = false;

	wstring read_dir = L"";
	wstring write_dir = L"";
	path path_read;

	if (argc > 1) {
		read_dir = s2ws(argv[1]);
	}
	else {
		cout << "Введите каталог >>> ";
		getline(wcin, read_dir);
	}
	read_dir = parsLine(read_dir);
	if (argc > 2) {
		write_dir = s2ws(argv[2]);
	}
	else {
		write_dir = L".//";
	}
	write_dir = parsLine(write_dir);
	transform(read_dir.begin(), read_dir.end(), read_dir.begin(), towlower);
	map <wstring, wstring> enviroment = {
		{L"%appdata%",L"appdata"},
		{L"%localappdata%",L"localappdata"},
	};
	for (pair<wstring, wstring> env : enviroment) {
		size_t pos = read_dir.find(env.first);
		if (pos != wstring::npos) {
			wchar_t* pvalue;
			size_t len;
			errno_t err = _wdupenv_s(&pvalue, &len, env.second.c_str());
			if (err != 0 || pvalue == nullptr) {
				wcout << L"Не удается прочитать переменную среды" << endl << L"Попробуйте написать путь без " << env.first << endl;
				system("pause");
				return 0;
			}
			read_dir.replace(pos, env.first.length(), wstring(pvalue));
		}
	}
	if (findfile(read_dir, path_read)) {
		wcout << L"Файл найден! " << path_read.wstring() << endl;
	}
	else {
		cout << "Файл не найден!" << endl;
		system("pause");
		return 0;
	}

	time_t curr_time;
	tm curr_tm;
	wchar_t date_string[100];
	time(&curr_time);
	localtime_s(&curr_tm, &curr_time);
	wcsftime(date_string, 50, L"%H-%M-%S_%d-%m-%g", &curr_tm);
	wstring path_write = write_dir + date_string + L".sql";

	long stroke = 0;
	wifstream fin;																			//	Входной файловый поток

	fin.open(path_read.wstring());

	if (!fin.is_open()) {																	// Проверяем правильность открытия файла
		cout << "Ошибка открытия файла вводимых данных!" << endl;
		fin.close();
		system("pause");
		return 0;
	}
	cout << "Файл вводимых данных открыт успешно!" << endl;

	wstring str;

	if (!fin.eof()) {																		//	Просмотр первой строки, нахождение заголовка файла
		stroke++;
		getline(fin, str);
		size_t pos = str.find(L"SimHistory swerrors");
		if (pos == wstring::npos) {
			cout << "Заголовок не совпадает!" << endl;
			system("pause");
			return 0;
		}
	}
	wcout << path_write;
	wofstream write_query;
	write_query.open(path_write);
	write_query.imbue(locale("en_US.utf-8"));
	if (!write_query.is_open()) {
		cout << "Ошибка открытия файла для записи" << endl;
		system("pause");
		return 0;
	}

	double start_time = clock();

	write_query << L"BEGIN IMMEDIATE TRANSACTION;" << endl;
	while (!fin.eof()) {
		stroke++;
		getline(fin, str);
		try {
			size_t found = str.find_first_not_of(L"1234567890;");							//	Проверка на наличие 'левых' символов			
			if (found != string::npos) {
				throw 0;
			}
			if (str == L"") {																//	Проверяем, что строка не пустая (как в конце файла)
				continue;
			}
			wstring temp;
			std::vector<std::wstring> arr;
			std::wstringstream wss(str);
			while (std::getline(wss, temp, L';'))
				arr.push_back(temp);
			//	Запись запросов
			wstring query1 = L"INSERT INTO main('время записи','id в симуляторе','id в vipe','id комплекта',event_id,'порт','id нового комплекта') values(" + arr.at(0) + L"," + arr.at(1) + L"," + arr.at(2) + L"," + arr.at(3) + L"," + arr.at(4) + L",";
			wstring query2 = L"";
			wstring query3 = L"";
			switch (event(stol(arr.at(4))))
			{
			case DEVICE_FAILURE:
				query1 += L"null,null);";
				break;
			case PORT_FAILURE:
				query1 += arr.at(5) + L",null);";
				break;
			case UNIT_SWITCH:
				query1 += L"null," + arr.at(5) + L");";
				break;
			case CHANNEL_ERROR:
				query2 = L"INSERT INTO channel_error values((SELECT MAX(id) from main),'" + arr.at(5) + L"','" + arr.at(6) + L"'," + arr.at(7) + L",";
				query1 += L"null,null);";
				switch (mechanism_type(stol(arr.at(7))))
				{
				case PER:
					if (type_id.count(arr.at(13)) == 0) {
						throw 1;
					}
					query3 = L"INSERT INTO per values((SELECT MAX(id) from main)," + arr.at(8) + L"," + arr.at(9) + L"," + arr.at(10) + L"," + arr.at(11) + L",'" + type_id[arr.at(13)] + L"'," + arr.at(15) + L"," + arr.at(15) + L");";
					query2 += arr.at(12) + L");";
					break;

				case BER:
					if (reason.count(arr.at(9)) == 0) {
						throw 2;
					}
					query3 = L"INSERT INTO ber values((SELECT MAX(id) from main)," + arr.at(8) + L",'" + reason[arr.at(9)] + L"',";
					switch (error_reason(stol(arr.at(9))))
					{
					case BIT_DISTORTION:
					case BIT_DISTORTION_AND_CONSEQUENCE_PREVIOUS_ERROR:
						if (type_SpaceWire_id.count(arr.at(12)) == 0) {
							throw 3;
						}
						query3 += arr.at(10) + L"," + arr.at(11) + L",'" + type_SpaceWire_id[arr.at(12)] + L"');";
						if (arr.at(12) == L"8") {
							query2 += arr.at(13) + L");";
						}
						else {
							query2 += L"null);";
						}
						break;

					case CONSEQUENCE_PREVIOUS_ERROR:
						query2 += L"null);";
						query3 += L"null,null,null);";
						break;
					default:
						throw 2;
						break;
					}
					break;
				default:
					throw 4;
					break;
				}
				break;
			default:
				throw 5;
				break;
			}
			if (!is_main_created) {
				write_query << main_create;
				is_main_created = true;
			}
			if (query2 != L"") {
				if (!is_ch_err_created) {
					write_query << ch_err_create;
					is_ch_err_created = true;
				}
				if (!is_ber_created && arr.at(7) == L"1") {
					write_query << ber_create;
					is_ber_created = true;
				}
				if (arr.at(7) == L"0" && !(is_per_created)) {
					write_query << per_create;
					is_per_created = true;
				}
			}
			write_query << query1 << endl << query2 << endl << query3 << endl;
		}
		catch (std::out_of_range) {
			error_occurred(-1, stroke, write_query, errors_create, is_errors_created);
		}
		catch (const int ex) {
			error_occurred(ex, stroke, write_query, errors_create, is_errors_created);
		}
	}
	double end_time = clock();
	double result_time = end_time - start_time;
	cout << result_time / CLK_TCK << " секунд\n";
	write_query << L"COMMIT TRANSACTION;" << endl;
	fin.close();
	write_query.close();
	system("pause");
	return 0;
}
