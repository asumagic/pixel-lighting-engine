#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>

void rgb_to_indexed(sf::Image& image, const sf::Image& palette_image)
{
	auto find_matching = [&](sf::Color color) -> int {
		for (unsigned i = 0; i < palette_image.getSize().x; ++i)
		{
			if (color == palette_image.getPixel({i, 0u}))
			{
				return i;
			}
		}

		return -1;
	};

	for (unsigned y = 0; y < image.getSize().y; ++y)
	{
		for (unsigned x = 0; x < image.getSize().x; ++x)
		{
			auto match = find_matching(image.getPixel({x, y}));

			if (match == -1)
			{
				std::cout << "MISSING PALETTE COLOR: (" << x << "; " << y
				          << ")!\n";

				continue;
			}

			image.setPixel({x, y}, {std::uint8_t(match), 0, 0});
		}
	}
}

void load_texture_indexed(
    sf::Texture&      texture,
    const sf::String& path,
    const sf::Image&  palette_image)
{
	std::cout << "Loading indexed texture from file " << path.toAnsiString()
	          << '\n';

	sf::Image image;
	image.loadFromFile(path.toAnsiString());
	rgb_to_indexed(image, palette_image);

	texture.loadFromImage(image);
}

int main()
{
	sf::RenderWindow win{sf::VideoMode{{1920, 1080}}, "Lighting shader test"};

	win.setVerticalSyncEnabled(true);
	win.setMouseCursorVisible(false);
	win.setMouseCursorGrabbed(false);

	float light_scale = 0.5f;

	// Light texture
	sf::Texture light_texture;
	light_texture.loadFromFile("light.png");
	sf::Sprite cursor_light_sprite{light_texture};
	cursor_light_sprite.setOrigin(sf::Vector2f(light_texture.getSize()) / 2.0f);

	// Textures used for full-screen shader effects
	sf::RenderTexture light_composite_texture(win.getSize());
	sf::RenderTexture composite_texture(win.getSize());

	sf::View view = win.getView();
	view.zoom(0.5);
	view.setCenter({500, 300});
	light_composite_texture.setView(view);
	composite_texture.setView(view);

	std::vector<sf::Sprite> light_sprites;

	// Generalistic textures
	sf::Texture palette_texture;
	sf::Texture palette_light_texture;
	sf::Texture tile_texture;
	sf::Texture turret_texture;

	{
		sf::Image palette_image;
		palette_image.loadFromFile("palette.png");

		palette_texture.loadFromImage(palette_image);

		load_texture_indexed(
		    palette_light_texture, "palette_light_shift.png", palette_image);
		load_texture_indexed(tile_texture, "tile.png", palette_image);
		load_texture_indexed(turret_texture, "turret.png", palette_image);
	}

	sf::Shader compose_shader;
	compose_shader.loadFromFile("compose.vert", "compose.frag");
	compose_shader.setUniform("palette_texture", palette_texture);
	compose_shader.setUniform("screen_texture", composite_texture.getTexture());
	compose_shader.setUniform(
	    "lightmap_texture", light_composite_texture.getTexture());
	compose_shader.setUniform("palette_shift_texture", palette_light_texture);

	while (win.isOpen())
	{
		while (auto ev = win.pollEvent())
		{
			if (ev->is<sf::Event::Closed>())
			{
				win.close();
			}
			else if (
			    const auto press = ev->getIf<sf::Event::MouseButtonPressed>())
			{
				if (press->button == sf::Mouse::Button::Left)
				{
					light_sprites.push_back(cursor_light_sprite);
				}
			}
			else if (
			    const auto scroll = ev->getIf<sf::Event::MouseWheelScrolled>())
			{
				light_scale += scroll->delta * 0.025;
			}
			else if (const auto press = ev->getIf<sf::Event::KeyPressed>())
			{
				switch (press->code)
				{
				case sf::Keyboard::Key::Add: {
					auto current_color = cursor_light_sprite.getColor();
					current_color.a += 8;
					cursor_light_sprite.setColor(current_color);
					break;
				}

				case sf::Keyboard::Key::Subtract: {
					auto current_color = cursor_light_sprite.getColor();
					current_color.a -= 8;
					cursor_light_sprite.setColor(current_color);
					break;
				}

				default: break;
				}
			}
		}

		cursor_light_sprite.setPosition(sf::Vector2f(sf::Mouse::getPosition()));
		cursor_light_sprite.setScale({light_scale, light_scale});

		win.clear({34, 32, 52});

		{
			composite_texture.clear({31, 0, 0, 255});

			// Draw tiles
			for (unsigned y : {256, 256 + 128})
			{
				for (unsigned x = 0; x < 800; x += 16)
				{
					sf::Sprite sprite(tile_texture);

					if (y == 256)
					{
						sprite.setScale({1.0f, -1.0f});
					}

					sprite.setPosition({float(x), float(y)});
					composite_texture.draw(sprite);
				}
			}

			sf::Sprite sprite{turret_texture};
			sprite.setPosition(
			    {256.0f, 256.0f + 128.0f - turret_texture.getSize().y});
			composite_texture.draw(sprite);

			composite_texture.display();
		}

		// Draw lights to composite texture
		{
			light_composite_texture.clear();

			sf::RenderStates states;
			states.blendMode = sf::BlendAdd;

			light_composite_texture.draw(cursor_light_sprite, states);

			for (const auto& sprite : light_sprites)
			{
				light_composite_texture.draw(sprite, states);
			}

			light_composite_texture.display();
		}

		// Post-processing: Apply lighting and convert from indexed to color
		{
			sf::RenderStates states;
			states.shader = &compose_shader;

			sf::Sprite composite_sprite{composite_texture.getTexture()};

			win.draw(composite_sprite, states);
		}

		win.display();
	}
}
