#include "Collision.h"
using namespace DirectX;

bool Collision::CheckSphere2Plane(const Sphere& sphere, const Plane& plane, DirectX::XMVECTOR* inter){
	//座標系の原点から球の中心座標への距離
	XMVECTOR distV = XMVector3Dot(sphere.center, plane.normal);
	//平面の原点座標を減算することで、平面と球の中心との距離が出る
	float dist = distV.m128_f32[0] - plane.distance;
	//距離の絶対値が半径より大きければ当たっていない
	if (fabs(dist) > sphere.radius) return false;
	//疑似交点を計算
	if (inter) {
		//平面上の最近接点を疑似交点とする
		*inter = -dist * plane.normal + sphere.center;
	}
	return true;
}
