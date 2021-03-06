#ifndef SOLUTION_TRADER_TRADER_HPP
#define SOLUTION_TRADER_TRADER_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#  pragma once
#endif // #ifdef BOOST_HAS_PRAGMA_ONCE

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <exception>
#include <execution>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <mutex>
#include <ostream>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/extended/application/service.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include "stream/stream.hpp"

#include "../market/market.hpp"

#include "../../../shared/source/logger/logger.hpp"

namespace solution 
{
	namespace trader
	{
		class trader_exception : public std::exception
		{
		public:

			explicit trader_exception(const std::string & message) noexcept : 
				std::exception(message.c_str()) 
			{}

			explicit trader_exception(const char * const message) noexcept :
				std::exception(message)
			{}

			~trader_exception() noexcept = default;
		};

		class Trader : public boost::extended::application::service
		{
		private:

			using assets_container_t = std::vector < std::string > ;
			using scales_container_t = std::vector < std::string > ;

			using clock_t = std::chrono::system_clock;

			using time_point_t = clock_t::time_point;

		private:

			struct Point
			{
				time_point_t time;
				double price;
			};

		public:

			enum class Level_Resolution
			{
				day,
				week,
				month,
			};

		private:

			struct Level
			{
				time_point_t time;
				double price;
				std::size_t strength;
			};

		private:

			using levels_container_t = std::map < std::string, 
				std::unordered_map < Level_Resolution, std::vector < Level > > > ;

		private:

			class Data
			{
			private:

				struct File
				{
					static inline const std::filesystem::path assets = "trader/data/assets.data";
					static inline const std::filesystem::path scales = "trader/data/scales.data";
				};

			public:

				static void load_assets(assets_container_t & assets);
				static void load_scales(scales_container_t & scales);
			};

		public:

			struct Candle
			{
				using date_t = std::string;
				using time_t = std::string;

				using price_t = double;

				using volume_t = std::uint64_t;

				date_t date;
				time_t time;

				price_t price_open;
				price_t price_high;
				price_t price_low;
				price_t price_close;

				volume_t volume;
			};

		private:

			template < typename Iterator >
			class Candle_Parser :
				public boost::spirit::qi::grammar < Iterator, Candle(), boost::spirit::qi::blank_type >
			{
			private:

				using rule_t = boost::spirit::qi::rule < Iterator, Candle(), boost::spirit::qi::blank_type > ;

			public:

				Candle_Parser() : Candle_Parser::base_type(start)
				{
					static const auto separator = ',';

					start %=
						+(boost::spirit::qi::char_ - separator) >> separator >>
						+(boost::spirit::qi::char_ - separator) >> separator >>
						  boost::spirit::qi::double_			>> separator >>
						  boost::spirit::qi::double_			>> separator >>
						  boost::spirit::qi::double_			>> separator >>
						  boost::spirit::qi::double_			>> separator >>
						  boost::spirit::qi::ulong_long;
				}

				~Candle_Parser() noexcept = default;

			private:

				rule_t start;
			};

		private:

			struct Extension
			{
				using extension_t = std::string;

				static inline const extension_t txt = ".txt";
				static inline const extension_t csv = ".csv";
				static inline const extension_t dat = ".dat";

				static inline const extension_t empty = "";
			};

		private:

			class Scoped_Shared_Lock
			{
			public:

				using mutex_t = std::shared_mutex;

			public:

				explicit Scoped_Shared_Lock(mutex_t & mutex)
					: m_mutex(mutex)
				{
					m_mutex.lock_shared();
				}

				~Scoped_Shared_Lock() noexcept
				{
					m_mutex.unlock_shared();
				}

			private:

				Scoped_Shared_Lock			  (const Scoped_Shared_Lock &) = delete;
				Scoped_Shared_Lock & operator=(const Scoped_Shared_Lock &) = delete;

			private:

				mutex_t & m_mutex;
			};

		public:

			Trader() :
				m_thread_pool(std::thread::hardware_concurrency())
			{
				initialize();
			}

			~Trader() noexcept = default;

		private:

			void initialize();

		private:

			void load();

		private:

			void load_assets();
			void load_scales();

			std::vector < Level > reduce_levels(std::vector < Level > && levels) const;

			std::vector < Level > make_levels(
				const std::vector < Point > & points, Level_Resolution level_resolution) const;

			std::vector < Point > make_points(const std::filesystem::path & path) const;

			Point parse(const std::string & line) const;

			time_point_t parse(const Candle::date_t & date, const Candle::time_t & time) const;

			std::size_t level_resolution_to_frame(Level_Resolution level_resolution) const;

			void save_levels(const std::string & asset) const;

		public:

			void print_levels(
				const std::string & asset, Level_Resolution level_resolution) const;

			std::string level_resolution_to_string(Level_Resolution level_resolution) const;

		private:

			friend std::ostream & operator<<(std::ostream & stream, const Level & level);

		public:

			virtual void run() override;

			virtual void stop() override;

		private:

			void run_implementation() const;

			bool is_session_open() const;

		private:

			static inline const std::filesystem::path directory = "trader/levels";

			static inline const std::string initial_scale = "M60";

			static inline const double price_deviation = 0.0025;

		private:

			assets_container_t m_assets;
			scales_container_t m_scales;

			Market m_market;

			levels_container_t m_levels;

			boost::asio::thread_pool m_thread_pool;

		private:

			mutable std::shared_mutex m_levels_mutex;
		};

	} // namespace trader

} // namespace solution

BOOST_FUSION_ADAPT_STRUCT
(
	 solution::trader::Trader::Candle,
	(solution::trader::Trader::Candle::date_t,   date)
	(solution::trader::Trader::Candle::time_t,   time)
	(solution::trader::Trader::Candle::price_t,  price_open)
	(solution::trader::Trader::Candle::price_t,  price_high)
	(solution::trader::Trader::Candle::price_t,  price_low)
	(solution::trader::Trader::Candle::price_t,  price_close)
	(solution::trader::Trader::Candle::volume_t, volume)
)

#endif // #ifndef SOLUTION_TRADER_TRADER_HPP