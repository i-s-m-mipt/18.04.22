//#define LUA_LIB

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
//#  define LUA_BUILD_AS_DLL
//#endif // #if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <tuple>

#include <qluacpp/qlua>

#include "market/market.hpp"

#include "../../shared/source/logger/logger.hpp"

namespace solution
{
	namespace plugin
	{
		using Logger = shared::Logger;

		static struct luaL_reg functions[] =
		{
			{ NULL, NULL }
		};

		static std::unique_ptr < Market > market_ptr;

		void main(lua::state & state) 
		{
			RUN_LOGGER(logger);

			try
			{
				Market::api_t api(state);

				market_ptr = std::make_unique < Market > (api);

				market_ptr->run();
			}
			catch (const std::exception & exception)
			{
				logger.write(Logger::Severity::fatal, exception.what());
			}
			catch (...)
			{
				logger.write(Logger::Severity::fatal, "unknown exception");
			}
		}

		std::tuple < int > stop(const lua::state & state, lua::entity < lua::type_policy < int > > signal)
		{
			market_ptr.reset();

			return std::make_tuple(int(1));
		}

	} // namespace plugin

} // namespace solution

LUACPP_STATIC_FUNCTION2(main,    solution::plugin::main)
LUACPP_STATIC_FUNCTION3(OnStop,  solution::plugin::stop, int)

extern "C" 
{
	LUALIB_API int luaopen_plugin(lua_State * state_ptr) 
	{
		lua::state state(state_ptr);

		lua::function::main   ().register_in_lua(state, solution::plugin::main);
		lua::function::OnStop ().register_in_lua(state, solution::plugin::stop);

		luaL_openlib(state_ptr, "plugin", solution::plugin::functions, 0);

		return 0;
	}
}

// =======================================================================================================================

//#include <chrono>
//#include <memory>
//#include <thread>
//
//#include <qluacpp/qlua>
//
//struct wa_table {
//	// Creates QLUA table
//	void create(const qlua::api& q, const char* name);
//	// Updates table with the data and timestamps it
//	void update(const qlua::api& q,
//		const double bid_wap, const double bid_quant,
//		const double ask_wap, const double ask_quant);
//
//private:
//	int table_id_{ 0 };
//
//	// Table column ids
//	const int col_desc_{ 1 };
//	const int col_value_{ 2 };
//
//	// Table row ids
//	int row_time_{ -1 };
//	int row_bid_wap_{ -1 };
//	int row_bid_quant_{ -1 };
//	int row_ask_wap_{ -1 };
//	int row_ask_quant_{ -1 };
//};
//
//#include <chrono>
//#include <ctime>
//#include <sstream>
//
//void wa_table::create(const qlua::api& q, const char* name) {
//	// Allocate Qlua table
//	table_id_ = q.AllocTable();
//	if (table_id_ == 0) {
//		q.message("l2q_wa: Could not alloc table");
//		return;
//	}
//	// Get Qlua constant value
//	auto qtable_string_type = q.constant<unsigned int>("QTABLE_STRING_TYPE");
//	// Create table's columns
//	auto rc = q.AddColumn(table_id_, col_desc_, "Desc", true, qtable_string_type, 25);
//	rc = rc * q.AddColumn(table_id_, col_value_, "Value", true, qtable_string_type, 35);
//	if (!rc) {
//		q.message("l2q_wa: Could not create table columns");
//		return;
//	}
//	// Create table's window and set caption
//	if (q.CreateWindow(table_id_)) {
//		q.SetWindowCaption(table_id_, name);
//	}
//	else {
//		q.message("l2wa_wa: Could not create window");
//	}
//	// Create rows
//	row_time_ = q.InsertRow(table_id_, -1);
//	row_bid_wap_ = q.InsertRow(table_id_, -1);
//	row_bid_quant_ = q.InsertRow(table_id_, -1);
//	row_ask_wap_ = q.InsertRow(table_id_, -1);
//	row_ask_quant_ = q.InsertRow(table_id_, -1);
//	// Create labels for each row
//	q.SetCell(table_id_, row_time_, col_desc_, "Time");
//	q.SetCell(table_id_, row_bid_wap_, col_desc_, "Bid WAP");
//	q.SetCell(table_id_, row_bid_quant_, col_desc_, "Bid vol");
//	q.SetCell(table_id_, row_ask_wap_, col_desc_, "Ask WAP");
//	q.SetCell(table_id_, row_ask_quant_, col_desc_, "Ask vol");
//}
//
//void wa_table::update(const qlua::api& q,
//	const double bid_wap, const double bid_quant,
//	const double ask_wap, const double ask_quant) {
//	// Create time string for the timestamp
//	using std::chrono::system_clock;
//	// Lambda to remove unneeded characters from the end of time strings
//	auto remove_crlf = [](char c[]) {
//		const auto len = strlen(c);
//		while ((len > 0) && ((c[len - 1] == '\n') || (c[len - 1] == '\r')))
//			c[len - 1] = 0x00;
//	};
//	std::time_t tt = system_clock::to_time_t(system_clock::now());
//	char timec_event[256];
//	ctime_s(timec_event, 256, &tt);
//	remove_crlf(timec_event);
//
//	// Simple lambda to format doubles as strings
//	auto double2string = [](const double d) {
//		std::stringstream ss;
//		ss << d;
//		return ss.str();
//	};
//
//	// Set table values
//	q.SetCell(table_id_, row_time_, col_value_, timec_event);
//	q.SetCell(table_id_, row_bid_wap_, col_value_, double2string(bid_wap).c_str());
//	q.SetCell(table_id_, row_bid_quant_, col_value_, double2string(bid_quant).c_str());
//	q.SetCell(table_id_, row_ask_wap_, col_value_, double2string(ask_wap).c_str());
//	q.SetCell(table_id_, row_ask_quant_, col_value_, double2string(ask_quant).c_str());
//}
//
//static struct luaL_reg ls_lib[] = {
//  { NULL, NULL }
//};
//
//static size_t depth_count{ 3 };
//static bool done{ false };
//static std::string sec_class{ "TQBR" };
//static std::string sec_code{ "SBER" };
//static wa_table table;
//
//void OnQuote(const lua::state& l,
//	const char* quote_sec_class,
//	const char* quote_sec_code);
//
//void my_main(lua::state& l) {
//	using namespace std::chrono_literals;
//	qlua::api q(l);
//	table.create(q, ("Average L2Q for " + sec_class + ":" + sec_code + ", depth " + std::to_string(depth_count)).c_str());
//	if (q.Subscribe_Level_II_Quotes("TQBR", "SBER")) {
//		while (!done) {
//			std::this_thread::sleep_for(100ms);
//			try
//			{
//				OnQuote(l, "TQBR", "SBER");
//			}
//			catch (const std::exception & e)
//			{
//				q.message(e.what());
//			}
//		}
//	}
//	else {
//		q.message(("Could not subscribe to " + sec_class + ":" + sec_code).c_str());
//	}
//}
//
//void OnQuote(const lua::state& l,
//	const char* quote_sec_class,
//	const char* quote_sec_code) 
//{
//	qlua::api q(l);
//	q.message("entered OnQuote");
//	if ((quote_sec_class == sec_class) && (quote_sec_code == sec_code)) {
//		
//		q.getQuoteLevel2(quote_sec_class, quote_sec_code,
//			[&q](const ::qlua::table::level2_quotes& quotes) {
//			auto bid = quotes.bid();
//			auto offer = quotes.offer();
//			double bid_quant{ 0.0 };
//			double bid_wap{ 0.0 };
//			double offer_quant{ 0.0 };
//			double offer_wap{ 0.0 };
//			if ((depth_count <= bid.size()) && (depth_count <= offer.size())) {
//				for (size_t i = 0; i < depth_count; ++i) {
//					auto& b = bid[bid.size() - 1 - i];
//					const double bq = atof(b.quantity.c_str());
//					bid_quant += bq;
//					bid_wap += atof(b.price.c_str()) * bq;
//					auto& o = offer[i];
//					const double oq = atof(o.quantity.c_str());
//					offer_quant += oq;
//					offer_wap += atof(o.price.c_str()) * oq;
//				}
//				bid_wap = bid_wap / bid_quant;
//				offer_wap = offer_wap / offer_quant;
//				table.update(q, bid_wap, bid_quant, offer_wap, offer_quant);
//			}
//
//		});
//	}
//	q.message("exiting OnQuote");
//}
//
//LUACPP_STATIC_FUNCTION2(main, my_main)
//
//extern "C" {
//	LUALIB_API int luaopen_plugin(lua_State *L) {
//		lua::state l(L);
//
//		::lua::function::main().register_in_lua(l, my_main);
//
//		luaL_openlib(L, "plugin", ls_lib, 0);
//		return 0;
//	}
//}