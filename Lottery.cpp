#include "Lottery.hpp"

Lottery::Lottery() {
	// 初期値セット
	magnif.resize(GRADE_CNT);
	rooms.resize(GRADE_CNT);

	clear();

	std::random_device rnd;
	mt.seed(rnd());

}

Lottery::~Lottery() {
}

// 部屋番号のセット
bool Lottery::setRoomNumber(const char * fileName) {
	clear();

	if (fileName == nullptr) {
		return false;
	}

	using namespace std;

	ifstream ifs(fileName);
	if (!ifs) {
		return false;
	}

	string tmp;
	size_t num, grade;
	size_t i;
	while (getline(ifs, tmp)) {
		i = 0;

		grade = 0;
		while (tmp[i] >= '0' && tmp[i] <= '9') {
			grade *= 10;
			grade += tmp[i]-'0';
			i++;
		}

		while (tmp[i] == ' ' || tmp[i] == ',' || tmp[i] == '\t') i++;	// 区切り文字は飛ばす

		num = 0;
		while (tmp[i] >= '0' && tmp[i] <= '9') {	// 部屋番号取得
			num *= 10;
			num += tmp[i]-'0';
			i++;
		}

		while (tmp[i] == ' ' || tmp[i] == ',' || tmp[i] == '\t') i++;	// 区切り文字は飛ばす

		if (tmp[i] < '1' || tmp[i] > '9') {
			return false;
		}

		// 以下，正しいフォーマットの場合の処理

		RoomNum add;
		if (tmp[i] == '1') {	// 1人部屋
			add.number = num;
			add.id = 0;

			rooms[ tmp[0]-'0'-1 ].push_back(add);
		}
		else {					// 2人部屋
			add.number = num;
			add.id = 1;
			rooms[ tmp[0]-'0'-1 ].push_back(add);
			add.id = 2;
			rooms[ tmp[0]-'0'-1 ].push_back(add);
		}


		if (magnif[tmp[0]-'0'-1] == 0) {
			magnif[tmp[0]-'0'-1] = 1;	// 登録されたグループの倍率を1にする
		}

		if (tmp[0]-'0' > maxGrade) {
			maxGrade = tmp[0]-'0';
		}

		tmp.clear();

	}

	sumMagnif = 0;
	for (size_t i=0; i<magnif.size(); i++) {
		sumMagnif += magnif[i];
	}

	return true;
}

bool Lottery::setMagnification(size_t grade, size_t mag) {
	if (grade < 1 || grade > maxGrade) {
		return false;
	}

	magnif[grade-1] = mag;

	return true;
}


// 当選番号の取得
bool Lottery::getNumber(RoomNum& win) {
	return getNumber(win, 0);
}


// 当選番号の取得
bool Lottery::getNumber(RoomNum& win, int grade) {
	win.number = 0;
	win.id = 0;

	if (sumMagnif == 0 || maxGrade == 0) {
		return false;
	}
	if (grade > maxGrade) {
		return false;
	}

	if (grade > 0 ) {
		if (rooms[grade-1].size() == 0) {
			return false;
		}
	}
	else {
		int sum = 0;
		for (size_t i=0; i<rooms.size(); i++) {
			sum += rooms[i].size();
		}
		
		if (sum == 0) {
			return false;
		}
	}
	


	size_t selectGrade;
	size_t selectNum;
	
	size_t i=0;
	if (grade <= 0) {	// 学年指定なし
		std::vector<size_t> threshold(maxGrade);

		// 確率に基づいて，しきい値をセット
		// note:しきい値=人数x確率係数
		for (size_t i=0; i<maxGrade; i++) {
			if (i == 0)	threshold[i] = rooms[i].size()*magnif[i];
			else		threshold[i] = threshold[i-1] + rooms[i].size()*magnif[i];
		}

		// 乱数を発生させ，その乱数がどの領域にいるのか調べる
		unsigned int mtrand = mt()%threshold[maxGrade-1];
		
		for (size_t i=0; i<maxGrade; i++) {
			if (mtrand < threshold[i]) {
				selectGrade = i+1;
				break;
			}
		}

	}
	else {
		selectGrade = grade;
	}
	
	
	// 学年が決まったら，部屋番号を決める
	selectNum = mt()%rooms[selectGrade-1].size();

	win = rooms[selectGrade-1][selectNum];	// コピー


	// 当たった部屋番号の削除処理
	RoomNum tmp = rooms[selectGrade-1].back();
	rooms[selectGrade-1].back() = rooms[selectGrade-1][selectNum];
	rooms[selectGrade-1][selectNum] = tmp;
	rooms[selectGrade-1].pop_back();


	return true;
}


void Lottery::clear() {
	
	for (size_t i=0; i<rooms.size(); i++) {
		rooms[i].clear();
	}

	for (size_t i=0; i<magnif.size(); i++) {
		magnif[i] = 0;
	}

	sumMagnif = 0;
	maxGrade = 0;

}
