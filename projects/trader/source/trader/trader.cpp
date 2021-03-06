#include "trader.hpp"

namespace solution
{
	namespace trader
	{
		void Trader::Data::load_assets(assets_container_t & assets)
		{
			RUN_LOGGER(logger);

			try
			{
				std::fstream fin(File::assets.string(), std::ios::in);

				if (!fin)
				{
					throw trader_exception("cannot open file " + File::assets.string());
				}

				std::string asset;

				while (std::getline(fin, asset))
				{
					assets.push_back(asset);
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::Data::load_scales(scales_container_t & scales)
		{
			RUN_LOGGER(logger);

			try
			{
				std::fstream fin(File::scales.string(), std::ios::in);

				if (!fin)
				{
					throw trader_exception("cannot open file " + File::scales.string());
				}

				std::string scale;

				while (std::getline(fin, scale))
				{
					scales.push_back(scale);
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::initialize()
		{
			RUN_LOGGER(logger);

			try
			{
				std::filesystem::create_directory(directory);

				load();
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::load()
		{
			RUN_LOGGER(logger);

			try
			{
				load_assets();
				load_scales();

				using years = std::chrono::duration < int, std::ratio < 60 * 60 * 24 * 365 > > ;
				
				auto first = Market::clock_t::now() - years(1);
				auto last  = Market::clock_t::now();

				std::vector < std::future < void > > futures;

				futures.reserve(m_assets.size());

				for (const auto & asset : m_assets)
				{
					for (const auto & scale : m_scales)
					{
						if (scale == initial_scale)
						{
							std::packaged_task < void() > task([this, asset, scale, first, last]() 
							{
								auto points = make_points(m_market.get(asset, scale, first, last));

								auto week_levels = 
									reduce_levels(make_levels(points, Level_Resolution::week));
								auto month_levels = 
									reduce_levels(make_levels(points, Level_Resolution::month));

								{
									std::scoped_lock lock(m_levels_mutex);

									m_levels[asset][Level_Resolution::week ] = std::move(week_levels);
									m_levels[asset][Level_Resolution::month] = std::move(month_levels);
								}

								save_levels(asset);
							});

							futures.push_back(boost::asio::post(m_thread_pool, std::move(task)));
						}
					}
				}

				for (auto & future : futures)
				{
					future.wait();
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::load_assets()
		{
			RUN_LOGGER(logger);

			try
			{
				Data::load_assets(m_assets);
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::load_scales()
		{
			RUN_LOGGER(logger);

			try
			{
				Data::load_scales(m_scales);
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		std::vector < Trader::Level > Trader::reduce_levels(std::vector < Level > && levels) const
		{
			RUN_LOGGER(logger);

			try
			{
				if (levels.size() > 1)
				{
					for (auto first = levels.begin(); first != levels.end(); ++first)
					{
						for (auto current = std::next(first); current != levels.end();)
						{
							if (std::abs(first->price - current->price) / first->price <= price_deviation)
							{
								++first->strength;

								current = levels.erase(current);
							}
							else
							{
								++current;
							}
						}
					}
				}

				return levels;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		std::vector < Trader::Level > Trader::make_levels(
			const std::vector < Point > & points, Level_Resolution level_resolution) const
		{
			RUN_LOGGER(logger);

			try
			{
				auto frame = level_resolution_to_frame(level_resolution);

				std::vector < Level > levels;

				auto first = points.begin();

				switch (level_resolution)
				{
				case Level_Resolution::day:
					first = std::prev(points.end(), 9U * 5U * 4U);
					break;
				case Level_Resolution::week:
					first = std::prev(points.end(), 9U * 5U * 4U * 3U);
					break;
				case Level_Resolution::month:
					break;
				default:
					throw trader_exception("unknown level resolution");
					break;
				}

				for (; first != points.end(); )
				{
					auto last = std::next(first, std::min(frame, 
						static_cast < decltype(frame) > (std::distance(first, points.end()))));

					auto extremum = std::minmax_element(
						first, last, [](const auto & lhs, const auto & rhs)
					{
						return (lhs.price < rhs.price);
					});

					if ((extremum.first == first && first != points.begin() &&
						std::prev(first)->price > extremum.first->price) ||
						(extremum.first == std::prev(last) && last != points.end() &&
						last->price > extremum.first->price) ||
						(extremum.first != first && extremum.first != std::prev(last)))
					{
						levels.push_back(Level{ extremum.first->time, extremum.first->price, 0U });
					}

					if ((extremum.second == first && first != points.begin() &&
						std::prev(first)->price < extremum.second->price) ||
						(extremum.second == std::prev(last) && last != points.end() &&
						last->price > extremum.second->price) ||
						(extremum.second != first && extremum.second != std::prev(last)))
					{
						levels.push_back(Level{ extremum.second->time, extremum.second->price, 0U });
					}

					first = last;
				}

				return levels;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		std::vector < Trader::Point > Trader::make_points(
			const std::filesystem::path & path) const
		{
			RUN_LOGGER(logger);

			try
			{
				std::fstream fin(path.string(), std::ios::in);

				if (!fin)
				{
					throw trader_exception("cannot open file " + path.string());
				}

				std::vector < Point > plot;
				
				plot.reserve(365);

				std::string line;

				while (std::getline(fin, line))
				{
					plot.push_back(parse(line));
				}

				plot.shrink_to_fit();

				return plot;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		Trader::Point Trader::parse(const std::string & line) const
		{
			RUN_LOGGER(logger);

			try
			{
				Candle_Parser < std::string::const_iterator > parser;

				auto first = std::begin(line);
				auto last  = std::end  (line);

				Candle candle;

				auto result = boost::spirit::qi::phrase_parse(
					first, last, parser, boost::spirit::qi::blank, candle);

				if (result)
				{
					return Point { parse(candle.date, candle.time), candle.price_close };
				}
				else
				{
					throw trader_exception("cannot parse line " + line);
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		Trader::time_point_t Trader::parse(
			const Candle::date_t & date, const Candle::time_t & time) const
		{
			RUN_LOGGER(logger);

			try
			{
				std::tm tm;

				auto source = date + " " + time;

				std::istringstream sin(source);

				sin >> std::get_time(&tm, "%d/%m/%y %H:%M:%S");

				if (sin.fail())
				{
					throw trader_exception("cannot parse date-time " + source);
				}

				return clock_t::from_time_t(std::mktime(&tm));
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		std::size_t Trader::level_resolution_to_frame(Level_Resolution level_resolution) const
		{
			RUN_LOGGER(logger);

			try
			{
				switch (level_resolution)
				{
				case Level_Resolution::day:
					return 9U;
				case Level_Resolution::week:
					return 9U * 5U;
				case Level_Resolution::month:
					return 9U * 5U * 4U;
				default:
					throw trader_exception("unknown level resolution");
					break;
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::save_levels(const std::string & asset) const
		{
			RUN_LOGGER(logger);

			try
			{
				const std::filesystem::path file = asset + Extension::txt;
				const std::filesystem::path path = directory / file;

				std::fstream fout(path.string(), std::ios::out);

				if (!fout)
				{
					throw trader_exception("cannot open file " + path.string());
				}

				{
					Scoped_Shared_Lock lock(m_levels_mutex);

					for (const auto & level_resolution : m_levels.at(asset))
					{
						fout << "[" << asset << "] " <<
							"resolution: " << level_resolution_to_string(level_resolution.first) << std::endl;

						fout << std::endl;

						for (const auto & level : level_resolution.second)
						{
							fout << level << std::endl;
						}

						fout << std::endl;
					}
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::print_levels(
			const std::string & asset, Level_Resolution level_resolution) const
		{
			RUN_LOGGER(logger);

			try
			{
				std::cout << "[" << asset << "] " << 
					"resolution: " << level_resolution_to_string(level_resolution) << std::endl;

				std::cout << std::endl;

				for (const auto & level : m_levels.at(asset).at(level_resolution))
				{
					std::cout << level << std::endl;
				}

				std::cout << std::endl;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		std::string Trader::level_resolution_to_string(Level_Resolution level_resolution) const
		{
			RUN_LOGGER(logger);

			try
			{
				switch (level_resolution)
				{
				case Level_Resolution::day:
					return "D";
					break;
				case Level_Resolution::week:
					return "W";
					break;
				case Level_Resolution::month:
					return "M";
					break;
				default:
					throw trader_exception("unknown level resolution");
					break;
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		template < typename Duration, typename Clock >
		Duration duration_since_time_point(typename Clock::time_point time_point)
		{
			return std::chrono::duration_cast < Duration > (Clock::now() - time_point);
		}

		std::ostream & operator<<(std::ostream & stream, const Trader::Level & level)
		{
			RUN_LOGGER(logger);

			try
			{
				using days = std::chrono::duration < int, std::ratio < 60 * 60 * 24 > > ;

				auto time = decltype(level.time)::clock::to_time_t(level.time);

				auto tm = *std::localtime(&time);

				stream << 
					"price: " << std::setw(8) << std::setfill(' ') << std::right << 
						std::setprecision(2) << std::fixed << level.price << " " <<
					"since: " << std::put_time(&tm, "%y.%m.%d") << " "
					"alive: " << std::setw(3) << std::setfill(' ') << std::right <<
						duration_since_time_point < days, Trader::clock_t > (level.time).count() << " " <<
					"power: " << level.strength;

				return stream;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::run()
		{
			RUN_LOGGER(logger);

			try
			{
				std::cout << "Run trader ? (y/n) ";

				if (getchar() == 'y')
				{
					run_implementation();
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::stop()
		{
			RUN_LOGGER(logger);

			try
			{
				// ...
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		void Trader::run_implementation() const
		{
			RUN_LOGGER(logger);

			try
			{
				while (!is_session_open())
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}

				const auto w = 987U;
				const auto h = 330U;

				sf::VideoMode mode(w, h);

				sf::RenderWindow window(mode, "TRADER", sf::Style::Close | sf::Style::Titlebar);

				window.setFramerateLimit(30);

				sf::Font font;

				if (!font.loadFromFile("window/fonts/consolas.ttf"))
				{
					throw trader_exception("cannot load font");
				}

				sfe::Stream stream_levels; 
				sfe::Stream stream_states;

				stream_levels.setFont(font);
				stream_states.setFont(font);

				const auto character_size = 16U;

				stream_levels.setCharacterSize(character_size);
				stream_states.setCharacterSize(character_size);

				const auto stream_levels_width = character_size * 33;
				const auto indent = character_size / 8;

				stream_levels.setPosition(indent,                           0);
				stream_states.setPosition(indent * 2 + stream_levels_width, 0);

				sf::Vertex separator[] =
				{
					sf::Vertex(sf::Vector2f(indent + stream_levels_width, 0 + 2 * indent)),
					sf::Vertex(sf::Vector2f(indent + stream_levels_width, h - 2 * indent))
				};

				separator[0].color = sf::Color(211U, 211U, 211U);
				separator[1].color = sf::Color(211U, 211U, 211U);

				auto last_signal = clock_t::now();

				bool has_levels = false;

				while (window.isOpen())
				{
					sf::Event event;

					while (window.pollEvent(event))
					{
						if (event.type == sf::Event::Closed || !is_session_open())
						{
							window.close();
						}
					}

					stream_levels.clear();
					stream_states.clear();

					for (const auto & asset : m_levels)
					{
						const auto price = m_market.get_current_price(asset.first);

						std::ostringstream sout_level;
						std::ostringstream sout_state;

						sout_state << std::setprecision(2) << std::fixed << "[" << asset.first << "]" << 
							" price: " << std::setw(8) << std::setfill(' ') << std::right << 
								price <<
							" delta: " << std::setw(6) << std::setfill(' ') << std::right <<
								price / 100.0 * 1.0 * 0.25 <<
							" limit: " << std::setw(6) << std::setfill(' ') << std::right <<
								price / 100.0 * 1.0 << '\n';

						stream_states << sout_state.str();

						std::set < std::string, std::greater < std::string > > active_levels;

						for (const auto & level_resolution : asset.second)
						{
							for (const auto & level : level_resolution.second)
							{
								if (std::abs(level.price - price) / price <= price_deviation)
								{
									sout_level << "[" << asset.first << "] " << level << '\n';

									active_levels.insert(sout_level.str());

									sout_level.str("");

									has_levels = true;
								}
							}
						}

						for (const auto & active_level : active_levels)
						{
							stream_levels << active_level;
						}
					}

					if (has_levels && clock_t::now() - last_signal > std::chrono::seconds(60))
					{
						std::cout << '\a';

						last_signal = clock_t::now();

						has_levels = false;
					}

					window.clear();

					window.draw(stream_levels);
					window.draw(stream_states);

					window.draw(separator, 2, sf::Lines);

					window.display();
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

		bool Trader::is_session_open() const
		{
			RUN_LOGGER(logger);

			try
			{
				std::time_t time = std::time(nullptr);

				std::tm tm = *std::localtime(&time);

				return ((tm.tm_hour >= 10) && 
					((tm.tm_hour < 23) || (tm.tm_hour == 23 && tm.tm_min < 50)));
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < trader_exception > (logger, exception);
			}
		}

	} // namespace trader

} // namespace solution