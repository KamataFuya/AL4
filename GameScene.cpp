#include "GameScene.h"
#include <cassert>
#include "Collision.h"
#include <sstream>
#include <iomanip>

using namespace DirectX;

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
	delete spriteBG;
	delete object3d;
	delete model_;
	delete modelPlane_;
	delete modelTriangle_;
	delete modelHitSphere_;
}

void GameScene::Initialize(DirectXCommon* dxCommon, Input* input)
{
	// nullptrチェック
	assert(dxCommon);
	assert(input);

	this->dxCommon = dxCommon;
	this->input = input;

	// デバッグテキスト用テクスチャ読み込み
	Sprite::LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
	// デバッグテキスト初期化
	debugText.Initialize(debugTextTexNumber);

	// テクスチャ読み込み
	Sprite::LoadTexture(1, L"Resources/background.png");

	// 背景スプライト生成
	spriteBG = Sprite::Create(1, { 0.0f,0.0f });
	//OBJからモデルを読み込む
	model_ = Model::LoadFromOBJ("SphereBefore");
	modelPlane_ = Model::LoadFromOBJ("Plane");
	modelTriangle_ = Model::LoadFromOBJ("triangle_mat");
	modelHitSphere_ = Model::LoadFromOBJ("SphereAfter");
	// 3Dオブジェクト生成
	object3d = Object3d::Create();
	plane_ = Object3d::Create();
	triangle_ = Object3d::Create();
	hitSphere_ = Object3d::Create();
	//オブジェクトにモデルを紐づける
	object3d->SetModel(model_);
	plane_->SetModel(modelPlane_);
	triangle_->SetModel(modelTriangle_);
	hitSphere_->SetModel(modelHitSphere_);
	//テクスチャ2番に読み込み
	Sprite::LoadTexture(2, L"Resources/texture.png");
	//座標{0,0}に、テクスチャ2番のスプライトを生成
	sprite01 = Sprite::Create(2, { 0,0 });
	//座標{500,500}に、テクスチャ2番のスプライトを生成
	sprite02 = Sprite::Create(2, { 500,500 }, { 1,0,0,1 }, { 0,0 }, false, true);

	//球の初期値を設定
	sphere.center = XMVectorSet(0, 0, 0, 1); //中心座標
	sphere.radius = 6.0f; //半径

	//平面の初期値を設定
	plane.normal = XMVectorSet(0, 1, 0, 0); //法線ベクトル
	plane.distance = -15.0f; //原点(0,0,0)からの距離

	XMFLOAT3 planePosition = plane_->GetPosition();
	planePosition.y -= 15.0f;
	plane_->SetPosition(planePosition);

	//三角形の初期値を設定
	triangle.p0 = XMVectorSet(-1.0f, 0, -1.0f, 1); //左手前
	triangle.p1 = XMVectorSet(-1.0f, 0, +1.0f, 1); //左奥
	triangle.p2 = XMVectorSet(+1.0f, 0, -1.0f, 1); //右手前
	triangle.normal = XMVectorSet(0.0f,1.0f,0.0f,0); //上向き

	//レイの初期値を設定
	ray.start = XMVectorSet(0, 1, 0, 1); //原点やや上
	ray.dir = XMVectorSet(0, -1, 0, 0); //下向き
}

void GameScene::Update()
{
	//オブジェクトの移動
	// 現在の座標を取得
	XMFLOAT3 position = object3d->GetPosition();
	position.y += speedY;
	if (position.y >= 10.0f || position.y <= -30.0f) {
		speedY *= -1;
		moveCollisionY *= -1;
	}
	// 座標の変更を反映
	object3d->SetPosition(position);
	hitSphere_->SetPosition(position);
	//判定の移動
	//移動後の座標を計算
	sphere.center += moveCollisionY;

	// カメラ移動
	if (input->PushKey(DIK_W) || input->PushKey(DIK_S) || input->PushKey(DIK_D) || input->PushKey(DIK_A))
	{
		if (input->PushKey(DIK_W)) { Object3d::CameraMoveVector({ 0.0f,+1.0f,0.0f }); }
		else if (input->PushKey(DIK_S)) { Object3d::CameraMoveVector({ 0.0f,-1.0f,0.0f }); }
		if (input->PushKey(DIK_D)) { Object3d::CameraMoveVector({ +1.0f,0.0f,0.0f }); }
		else if (input->PushKey(DIK_A)) { Object3d::CameraMoveVector({ -1.0f,0.0f,0.0f }); }
	}

	//stringstreamで変数の値を埋め込んで整形する
	std::ostringstream spherestr;
	spherestr << "Sphere : ("
		<< std::fixed << std::setprecision(2) //小数点以下2桁まで
		<< sphere.center.m128_f32[0] << ","  //x
		<< sphere.center.m128_f32[1] << ","  //y
		<< sphere.center.m128_f32[2] << ")"; //z
	debugText.Print(spherestr.str(), 50, 180, 1.0f);

	////レイ操作
	//{
	//	XMVECTOR moveY = XMVectorSet(0, 0.01f,0, 0);
	//	if (input->PushKey(DIK_8)) {
	//		ray.start += moveY;
	//	}
	//	else if (input->PushKey(DIK_2)) {
	//		ray.start -= moveY;
	//	}

	//	XMVECTOR moveX = XMVectorSet(0.01f, 0, 0, 0);
	//	if (input->PushKey(DIK_6)) {
	//		ray.start += moveX;
	//	}
	//	else if (input->PushKey(DIK_4)) {
	//		ray.start -= moveX;
	//	}
	//}
	//std::ostringstream raystr;
	//raystr << "ray.start("
	//	<< std::fixed << std::setprecision(2)
	//	<< ray.start.m128_f32[0] << ","
	//	<< ray.start.m128_f32[1] << ","
	//	<< ray.start.m128_f32[2] << ")";
	//debugText.Print(raystr.str(), 50, 240, 1.0f);

	//球と平面の当たり判定
	{
		bool hit = Collision::CheckSphere2Plane(sphere, plane);
		if (hit) {
			debugText.Print("HIT", 50, 200, 1.0f);
			hitFlag = true;
		}
		else {
			hitFlag = false;
		}
	}
	//交点を算出するパターン --> 当たった場所にヒットエフェクトを発生させたい場合などに使う
	//{
	//	XMVECTOR inter;
	//	bool hit = Collision::CheckSphere2Plane(sphere, plane, &inter);
	//	if (hit) {
	//		debugText.Print("HIT", 50, 200, 1.0f);
	//		//stringstreamをリセットし、交点座標を埋め込む
	//		spherestr.str("""");
	//		spherestr.clear();
	//		spherestr << "("
	//			<< std::fixed << std::setprecision(2)
	//			<< inter.m128_f32[0] << ","
	//			<< inter.m128_f32[1] << ","
	//			<< inter.m128_f32[2] << ")";
	//		debugText.Print(spherestr.str(), 50, 220, 1.0f);
	//	}
	//}
	//球と三角形の当たり判定
	//{
	//	XMVECTOR inter;
	//	bool hit = Collision::CheckSphere2Triangle(sphere, triangle, &inter);
	//	if (hit) {
	//		debugText.Print("HIT", 50, 200, 1.0f);
	//		//stringstreamをリセットし、交点座標を埋め込む
	//		spherestr.str("""");
	//		spherestr.clear();
	//		spherestr << "("
	//			<< std::fixed << std::setprecision(2)
	//			<< inter.m128_f32[0] << ","
	//			<< inter.m128_f32[1] << ","
	//			<< inter.m128_f32[2] << ")";

	//		debugText.Print(spherestr.str(), 50, 220, 1.0f);
	//	}
	//}
	////レイと平面の当たり判定
	//{
	//	XMVECTOR inter;
	//	float distance;
	//	bool hit = Collision::CheckRay2Plane(ray, plane, &distance, &inter);
	//	if (hit) {
	//		debugText.Print("HIT", 50, 260, 1.0f);
	//		//stringstreamをリセットし、交点座標を埋め込む
	//		raystr.str("""");
	//		raystr.clear();
	//		raystr << "("
	//			<< std::fixed << std::setprecision(2)
	//			<< inter.m128_f32[0] << ","
	//			<< inter.m128_f32[1] << ","
	//			<< inter.m128_f32[2] << ")";
	//		debugText.Print(raystr.str(), 50, 280, 1.0f);
	//	}
	//}
	////レイと三角形の当たり判定
	//{
	//	float distance;
	//	XMVECTOR inter;
	//	bool hit = Collision::CheckRay2Triangle(ray, triangle, &distance, &inter);
	//	if (hit) {
	//		debugText.Print("HIT", 50, 300, 1.0f);
	//		//stringstreamをリセットし、交差座標を埋め込む
	//		raystr.str("""");
	//		raystr.clear();
	//		raystr << "inter : (" << std::fixed << std::setprecision(2)
	//			<< inter.m128_f32[0] << "," << inter.m128_f32[1] << "," << inter.m128_f32[2] << ")";
	//		debugText.Print(raystr.str(), 50, 320, 1.0f);

	//		raystr.str("""");
	//		raystr.clear();
	//		raystr << "distance : (" << std::fixed << std::setprecision(2) << distance << "(";
	//		debugText.Print(raystr.str(), 50, 340, 1.0f);
	//	}
	//}

	//スペースキーを押していたら
	if (input->PushKey(DIK_SPACE)) {
		//現在の座標を取得
		XMFLOAT2 position = sprite01->GetPosition();
		//移動後の座標を計算
		position.x += 1.0f;
		//座標の変更を反映
		sprite01->SetPosition(position);
	}

	//モデル更新処理
	object3d->Update();
	plane_->Update();
	triangle_->Update();
	hitSphere_->Update();
}

void GameScene::Draw()
{
	// コマンドリストの取得
	ID3D12GraphicsCommandList* cmdList = dxCommon->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(cmdList);
	// 背景スプライト描画
	spriteBG->Draw();

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる
	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	// 3Dオブジェクト描画前処理
	Object3d::PreDraw(cmdList);

	// 3Dオブクジェクトの描画
	if (hitFlag == false) { //当たっていない時は白い球
		object3d->Draw();
	}
	else { //当たっている時は赤い球
		hitSphere_->Draw();
	}
	plane_->Draw();

	/// <summary>
	/// ここに3Dオブジェクトの描画処理を追加できる
	/// </summary>

	// 3Dオブジェクト描画後処理
	Object3d::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(cmdList);

	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる
	/// </summary>
	// 描画
	/*sprite01->Draw();
	sprite02->Draw();*/

	// デバッグテキストの描画
	debugText.DrawAll(cmdList);

	// スプライト描画後処理
	Sprite::PostDraw();
#pragma endregion
}
