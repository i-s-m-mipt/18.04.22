#ifndef SOLUTION_PLUGIN_MARKET_HPP
#define SOLUTION_PLUGIN_MARKET_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#  pragma once
#endif // #ifdef BOOST_HAS_PRAGMA_ONCE

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <qluacpp/qlua> // !

#include <boost/extended/application/service.hpp>
#include <boost/extended/serialization/json.hpp>
#include <boost/filesystem/path.hpp>

#include "source/source.hpp"
#include "quotes/quotes.hpp"

#include "../../../shared/source/logger/logger.hpp"

namespace solution
{
	namespace plugin
	{
		class market_exception : public std::exception
		{
		public:

			explicit market_exception(const std::string & message) noexcept :
				std::exception(message.c_str())
			{}

			explicit market_exception(const char * const message) noexcept :
				std::exception(message)
			{}

			~market_exception() noexcept = default;
		};

		class Market : public boost::extended::application::service
		{
		private:

			using Source = market::Source;

			using Quotes = market::Quotes;

		public:

			using api_t = Source::api_t;

		private:

			using scales_container_t = std::vector < std::string > ;

			using assets_container_t = 
				std::vector < std::pair < std::string, std::string > > ;

			using sources_container_t = std::vector < std::shared_ptr < Source > > ;

			using quotess_container_t = std::vector < std::shared_ptr < Quotes > > ;

		private:

			class Data
			{
			private:

				using json_t = boost::extended::serialization::json;

			private:

				struct Key
				{
					struct Asset
					{
						static inline const std::string class_code = "class_code";
						static inline const std::string asset_code = "asset_code";
					};
				};

			private:

				struct File
				{
					using path_t = boost::filesystem::path;

					static inline const path_t assets = "market/data/assets.data";
					static inline const path_t scales = "market/data/scales.data";
				};

			private:

				using path_t = File::path_t;

			public:

				static void load(assets_container_t & assets);

				static void load(scales_container_t & scales);

			private:

				static void load(const path_t & path, json_t & object);				
			};

		public:

			enum class Status
			{
				running,
				stopped
			};

		private:

			/*
			class Task
			{
			public:

				Task(std::shared_ptr < Source > source, std::atomic < Status > & status) :
					m_source(source), m_status(status)
				{}

				~Task() noexcept = default;

			public:

				void operator()() const;

			private:

				static const inline std::size_t critical_error_quantity = 60;

			private:

				std::shared_ptr < Source > m_source;

			private:

				std::atomic < Status > & m_status;

			private:

				mutable std::size_t m_error_counter = 0;
			};
			*/

		public:

			explicit Market(const api_t & api)
			{
				initialize(api);
			}

			~Market() noexcept
			{
				try
				{
					uninitialize();
				}
				catch (...)
				{
					// std::abort();
				}
			}

		private:

			void initialize(const api_t & api);

			void uninitialize();

		private:

			void load();

		private:

			void load_assets();

			void load_scales();

		public:

			const auto & assets() const noexcept
			{
				return m_assets;
			}

			const auto & scales() const noexcept
			{
				return m_scales;
			}

		public:

			virtual void run() override;

			virtual void stop() override;

		private:

			assets_container_t m_assets;

			scales_container_t m_scales;

			sources_container_t m_sources;

			quotess_container_t m_quotess;

		private:

			mutable std::mutex m_mutex;

			mutable std::atomic < Status > m_status;
		};

	} // namespace plugin

} // namespace solution

#endif // #ifndef SOLUTION_PLUGIN_MARKET_HPP