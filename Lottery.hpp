#pragma once

#include <vector>
#include <fstream>
#include <random>
#include <string>

typedef struct _RoomNum {
	size_t number;
	size_t id;
}RoomNum;


class Lottery {
	std::vector<std::vector<RoomNum>> rooms;	// 部屋番号
	std::vector<int> magnif;					// 当選倍率
	
	size_t sumMagnif;
	
	size_t maxGrade;
	size_t gradeCnt;

	std::mt19937 mt;

	
public:
	Lottery();
	~Lottery();

	// 部屋番号のセット
	bool setRoomNumber(const char *fileName);
	
	// 当選倍率のセット
	bool setMagnification(size_t grade, size_t mag);

	// 当選番号の取得
	const RoomNum* getNumber(int grade);



};