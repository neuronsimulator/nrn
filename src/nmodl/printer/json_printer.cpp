#include "printer/json_printer.hpp"

/// By default dump output to std::cout
JSONPrinter::JSONPrinter() :
    result_stream(new std::ostream(std::cout.rdbuf()))
{}

// Dumpt output to stringstream
JSONPrinter::JSONPrinter(std::stringstream& ss) :
    result_stream(new std::ostream(ss.rdbuf()))
{}

/// Dump output to provided file
JSONPrinter::JSONPrinter(std::string fname) {
    if(fname.empty()) {
        throw std::runtime_error("Empty filename for JSONPrinter");
    }

    ofs.open(fname.c_str());

    if(ofs.fail()) {
        auto msg = "Error while opening file '" + fname + "' for JSONPrinter";
        throw std::runtime_error(msg);
    }

    sbuf = ofs.rdbuf();
    result_stream = std::make_shared<std::ostream>(sbuf);
}

/// Add node to json (typically basic type)
void JSONPrinter::addNode(std::string name) {
    if(!block) {
        auto text = "Block not initialized (pushBlock missing?)";
        throw std::logic_error(text);
    }

    json j;
    j["value"] = name;
    block->front().push_back(j);
}

/// Add new json object (typically start of new block)
/// name here is type of new block encountered
void JSONPrinter::pushBlock(std::string name) {
    if(block) {
        stack.push(block);
    }

    json j;
    j[name] = json::array();
    block = std::shared_ptr<json>(new json(j));
}

/// We finished processing a block, add processed block to it's parent block
void JSONPrinter::popBlock() {
    if (!stack.empty()) {
        auto current = block;
        block = stack.top();
        block->front().push_back(*current);
        stack.pop();
    }
}

/// Dump json object to stream (typically at the end)
/// nspaces is number of spaces used for indentation
void JSONPrinter::flush(int nspaces) {
    if(block) {
        if(nspaces) {
            *result_stream << (*block).dump(nspaces);
        } else {
            *result_stream << (*block).dump();
        }
        ofs.close();
        block = nullptr;
    }
}
