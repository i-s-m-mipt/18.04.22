#include "market.hpp"

namespace solution
{
	namespace mapper
	{
		using Severity = shared::Logger::Severity;

		void Market::Data::load_assets(assets_container_t & assets)
		{
			RUN_LOGGER(logger);

			try
			{
				auto path = File::assets;

				std::fstream fin(path.string(), std::ios::in);

				if (!fin)
				{
					throw market_exception("cannot open file " + path.string());
				}

				std::string asset;

				while (std::getline(fin, asset))
				{
					assets.push_back(asset);
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		void Market::Data::load_scales(scales_container_t & scales)
		{
			RUN_LOGGER(logger);

			try
			{
				auto path = File::scales;

				std::fstream fin(path.string(), std::ios::in);

				if (!fin)
				{
					throw market_exception("cannot open file " + path.string());
				}

				std::string scale;

				while (std::getline(fin, scale))
				{
					scales.push_back(scale);
				}
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		void Market::initialize()
		{
			RUN_LOGGER(logger);

			try
			{
				load();

				boost::filesystem::create_directory(directory);
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		void Market::load()
		{
			RUN_LOGGER(logger);

			try
			{
				load_assets();
				load_scales();
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		void Market::load_assets()
		{
			RUN_LOGGER(logger);

			try
			{
				Data::load_assets(m_assets);
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		void Market::load_scales()
		{
			RUN_LOGGER(logger);

			try
			{
				Data::load_scales(m_scales);
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		Market::path_t Market::get(const std::string & asset, const std::string & scale, time_point_t first, time_point_t last) const
		{
			RUN_LOGGER(logger);

			try
			{
				path_t path = asset + "_" + scale + Extension::txt;

				try
				{
					static Python_Initializer python_initializer;

					boost::python::object global = boost::python::import("__main__").attr("__dict__");

					boost::python::exec("from market import get", global, global);

					global["get"]
						(asset.c_str(), scale.c_str(), make_date(first).c_str(), make_date(last).c_str(), path.string().c_str());
				}
				catch (const boost::python::error_already_set &)
				{
					PyObject * error;
					PyObject * value;
					PyObject * stack;

					PyErr_Fetch				(&error, &value, &stack);
					PyErr_NormalizeException(&error, &value, &stack);

					boost::python::handle <> handle_error(error);

					boost::python::handle <> handle_value(boost::python::allow_null(value));
					boost::python::handle <> handle_stack(boost::python::allow_null(stack));
	
					std::string message = boost::python::extract < std::string > (
						!handle_value ? boost::python::str(handle_error) : boost::python::str(handle_value));

					logger.write(Severity::error, message);
				}

				return path;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
			catch (...)
			{
				shared::catch_handler < market_exception > (logger);
			}
		}

		Market::path_t Market::get(const std::string & asset, const std::string & scale) const
		{
			RUN_LOGGER(logger);

			try
			{
				using days = std::chrono::duration < int, std::ratio < 3600 * 24 > > ;

				const std::size_t total = 365 * 20;
				const std::size_t batch = 100;

				auto first = clock_t::now() - days(batch);
				auto last  = clock_t::now();

				auto path = directory; path /= asset + "_" + scale + Extension::txt;

				std::fstream fout(path.string(), std::ios::out | std::ios::trunc);

				if (!fout)
				{
					throw market_exception("cannot open file " + path.string());
				}

				for (std::size_t size = 0; size < total; size += batch)
				{
					auto temporary_file = get(asset, scale, first, last);

					std::fstream fin(temporary_file.string(), std::ios::in);

					if (!fin)
					{
						throw market_exception("cannot open file " + temporary_file.string());
					}

					std::string line;

					std::vector < std::string > lines;

					while (std::getline(fin, line))
					{
						lines.push_back(line);
					}

					fin.close();

					std::remove(temporary_file.string().c_str());

					std::for_each(std::rbegin(lines), std::rend(lines), [&fout](const auto & line)
					{
						fout << line << std::endl;
					});

					last = first - days(1);
					first = last - days(batch);

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}

				fout.close();

				return path;
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

		Market::date_t Market::make_date(time_point_t time_point) const
		{
			RUN_LOGGER(logger);

			try
			{
				auto time = clock_t::to_time_t(time_point);

				std::stringstream sout;

				sout << std::put_time(std::localtime(&time), "%y%m%d");

				return sout.str();
			}
			catch (const std::exception & exception)
			{
				shared::catch_handler < market_exception > (logger, exception);
			}
		}

	} // namespace mapper

} // namespace solution