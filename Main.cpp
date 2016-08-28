# include <Siv3D.hpp> // Siv3D August 2016

struct Grass
{
	Mat4x4 mat;
	ColorF color;
	Vec3 pos;
	double distance;
};

struct Ball
{
	Sphere sphere;
	bool found;
};

MeshData CreateGrassMesh()
{
	MeshData meshData;

	const Float3 nu(0, 1, 0), nd(0, -1, 0);

	for (auto i : step(3))
	{
		Mat4x4 mat = Mat4x4::Translate(0, 0, -0.2).rotated(0, Radians(i * 120), 0);
		meshData.vertices.push_back(MeshVertex{ mat.transform({-0.75, 0.5, 0}), nu, Float2(0,0) });
		meshData.vertices.push_back(MeshVertex{ mat.transform({ 0.75, 0.5, 0 }), nu, Float2(1,0) });
		meshData.vertices.push_back(MeshVertex{ mat.transform({ -0.75, 0, 0 }), nd, Float2(0,1) });
		meshData.vertices.push_back(MeshVertex{ mat.transform({ 0.75, 0, 0 }), nd, Float2(1,1) });
	}

	meshData.indices = { 0,1,2,2,1,3, 4,5,6,6,5,7, 8,9,10,10,9,11 };

	return meshData;
}

void Main()
{
	const ColorF skyColor(0.3, 0.7, 1);
	Window::SetTitle(L"Balls [W/A/S/D] 移動 [マウス] 方向 [クリック] ボールを拾う [F1] 低解像度/高解像度");
	Graphics::SetBackground(skyColor);

	Graphics3D::SetFog(Fog::SquaredExponential(skyColor, 0.04));
	Graphics3D::SetFogForward(Fog::SquaredExponential(skyColor, 0.04));
	Graphics3D::SetAmbientLight(ColorF(0.5));
	Graphics3D::SetAmbientLightForward(ColorF(0.5));
	
	const Vec3 lightDirection = Vec3(0.3, 1, 0.6).normalize();
	Graphics3D::SetLight(0, Light::Directional(lightDirection));
	Graphics3D::SetLightForward(0, Light::Directional(lightDirection));
	Graphics3D::SetShadowLight(ShadowLight(lightDirection * 60));

	// 草描画用のレンダーステート設定
	BlendState blend = BlendState::Default;
	blend.alphaToCoverageEnable = true;
	Graphics3D::SetBlendStateForward(blend);
	Graphics3D::SetRasterizerStateForward(RasterizerState::SolidCullNone);

	const Mesh meshGrass(CreateGrassMesh());
	const Texture textureGrass(Palette::White, L"grass.png", TextureDesc::For3D);

	const Mesh meshGround(MeshData::Plane(24, 24, { 20, 20 }));
	const Texture textureGround(L"Example/Grass.jpg", TextureDesc::For3D);

	const Model model(L"Example/Well/Well.wavefrontobj");

	const Sound soundBall(L"Ball.mp3");
	const Sound soundPark(L"Park.mp3", SoundLoop::All);
	const Sound soundWind(L"Wind.mp3", SoundLoop::All);

	soundPark.setVolume(0.15);
	soundPark.play(5s);

	soundWind.setVolume(0.05);
	soundWind.setSpeed(1.2);
	soundWind.play(3s);

	// 草の色と配置情報
	Array<Grass> grassList;
	for (auto p : step({31,31}))
	{
		const double x = (p.x - 15) * 0.6 + Random(-0.3, 0.3);
		const double z = (15 - p.y) * 0.6 + Random(-0.3, 0.3);

		if (Vec2(x, z).length() < 1.2)
		{
			continue;
		}

		Grass grass;
		grass.pos.set(x, 0, z);
		grass.mat = Mat4x4::Scale(1,Random(1.0, 1.3),1).rotated(0, Random(TwoPi), 0).translated(grass.pos);
		grass.color = HSV(100 + Random(16.0), 0.62 + Random(0.08), 0.66 + Random(0.12));
		grassList.push_back(grass);
	}

	// ボールの位置情報
	Array<Ball> balls;
	for (int32 i = 0; i < 10; ++i)
	{
		Vec2 pos = RandomVec2({ -10.0, 10.0 }, { -10.0, 10.0 });
		
		if (pos.length() < 0.5)
			pos.moveBy(2, 2);
		
		const double size = Random(0.08, 0.15);
		balls.push_back(Ball{ Sphere(pos.x, size, pos.y, size), false });
	}

	// プレイヤーカメラ
	double direction = 180_deg;
	Camera camera;
	camera.pos.set(0, 1.1, -10);

	// 残りボール・タイム表示
	const Font font(22, Typeface::Bold);
	Stopwatch stopwatch(true);

	while (System::Update())
	{
		if (Input::KeyF1.clicked)
		{
			Window::Resize(Window::Size() == Size(640, 480) ? Size(1280, 720) : Size(640, 480));
		}

		const double ax = (Window::Center().x - Mouse::Pos().x) / (0.5 * Window::Width());

		if (Abs(ax) > 0.2)
		{
			direction += Pow(Abs(ax - 0.2), 1.8) * Sign(ax) * 0.015;
		}

		const double speed = Input::KeyShift.pressed ? 0.03 : 0.01;
		const Vec2 front = Circular(speed, direction);
		const Vec2 left = Circular(speed, direction + 90_deg);

		if (Input::KeyW.pressed)
		{
			camera.pos.moveBy(front.x, 0, front.y);
		}
		else if (Input::KeyA.pressed)
		{
			camera.pos.moveBy(left.x, 0, left.y);
		}
		else if (Input::KeyS.pressed)
		{
			camera.pos.moveBy(-front.x, 0, -front.y);
		}
		else if (Input::KeyD.pressed)
		{
			camera.pos.moveBy(-left.x, 0, -left.y);
		}

		camera.pos.x = Clamp(camera.pos.x, -10.5, 10.5);
		camera.pos.z = Clamp(camera.pos.z, -10.5, 10.5);

		const double ay = (Window::Center().y - Mouse::Pos().y) / (0.5 * Window::Height());
		const Vec2 dir = Circular(4, direction);
		camera.lookat.set(camera.pos.x + dir.x, 0.9 + ay * 2.4, camera.pos.z + dir.y);

		Graphics3D::SetCamera(camera);

		meshGround.draw(textureGround);

		// 描画の品質のため、草を遠い順にソート
		const Vec3 cameraPos = Graphics3D::GetCamera().pos;
		for (auto& grass : grassList)
		{
			grass.distance = grass.pos.distanceFrom(cameraPos);
		}
		std::sort(grassList.begin(), grassList.end(), [](const Grass& a, const Grass& b){ return a.distance > b.distance; });

		Mat4x4 m = Mat4x4::Identity();
		const double t = Time::GetMillisec();	
		const ViewFrustum vf = Graphics3D::GetCamera().calcViewFrustum();

		for (const auto& grass : grassList)
		{
			// 遠い、または視野外の草は描画しない
			if (grass.distance > 20 || !vf.intersects(Sphere(grass.pos, 0.8)))
			{
				continue;
			}

			// 遠くの草のフェードアウト
			const double alpha = grass.distance > 17 ? (20 - grass.distance) / 3.0 : 1.0;

			// 風でなびく
			const double tx = (t + grass.pos.x * 100) * 0.00003;
			const double ty = (t + grass.pos.z * 100) * 0.000025 + 0.5_pi;
			m.r[1].m128_f32[0] = static_cast<float>(cos(tx*Pi) * cos(tx * 3_pi)*cos(tx * 5_pi)*cos(tx * 7_pi)*sin(tx * 25_pi)*0.5);
			m.r[1].m128_f32[2] = static_cast<float>(cos(ty*Pi) * cos(ty * 3_pi)*cos(ty * 5_pi)*cos(ty * 7_pi)*sin(ty * 25_pi)*0.5);
			
			meshGrass.drawForward(grass.mat * m, textureGrass, ColorF(grass.color, alpha));
		}

		model.draw(Mat4x4::Scale(0.5)).drawShadow(Mat4x4::Scale(0.5));

		for (int32 x = -11; x <= 11; ++x)
		{
			Box(x, 0.7, 11, 0.05, 1.4, 0.05).draw(HSV(30, 0.3, 0.6)).drawShadow();
			Box(x, 0.7, -11, 0.05, 1.4, 0.05).draw(HSV(30, 0.3, 0.6)).drawShadow();
		}

		for (int32 z = -10; z <= 10; ++z)
		{
			Box(-11, 0.7, z, 0.05, 1.4, 0.05).draw(HSV(30, 0.3, 0.6)).drawShadow();
			Box(11, 0.7, z, 0.05, 1.4, 0.05).draw(HSV(30, 0.3, 0.6)).drawShadow();
		}

		bool onBall = false;

		for (auto& ball : balls)
		{
			const bool touch = ball.sphere.center.distanceFrom(cameraPos) < 3.0 && ball.sphere.intersects(Mouse::Ray());

			ball.sphere.draw(touch ? Color(255, 160, 100) : Color(248)).drawShadow();

			if (touch)
			{
				onBall = true;

				if (Input::MouseL.clicked)
				{
					soundBall.playMulti();
					ball.found = true;
				}
			}
		}

		Cursor::SetStyle(onBall ? CursorStyle::Hand : CursorStyle::Default);

		Erase_if(balls, [](const Ball& ball) {return ball.found; });

		if (balls.empty())
		{
			stopwatch.pause();
		}

		font(L"見つかっていないボール: {}\nTIME: {}"_fmt, balls.size(), stopwatch.s())
			.draw(20, 20, AlphaF(balls.empty() ? System::FrameCount() %60 < 30 : 1));
	}
}
