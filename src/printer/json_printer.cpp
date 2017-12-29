#include "printer/json_printer.hpp"

/// Dump output to provided file
JSONPrinter::JSONPrinter(const std::string& filename) {
    if (filename.empty()) {
        throw std::runtime_error("Empty filename for JSONPrinter");
    }

    ofs.open(filename.c_str());

    if (ofs.fail()) {
        auto msg = "Error while opening file '" + filename + "' for JSONPrinter";
        throw std::runtime_error(msg);
    }

    sbuf = ofs.rdbuf();
    result = std::make_shared<std::ostream>(sbuf);
}

/// Add node to json (typically basic type)
void JSONPrinter::addNode(std::string value, const std::string& name) {
    if (!block) {
        auto text = "Block not initialized (pushBlock missing?)";
        throw std::logic_error(text);
    }

    json j;
    j[name] = value;
    block->front().push_back(j);
}

/// Add new json object (typically start of new block)
/// name here is type of new block encountered
void JSONPrinter::pushBlock(const std::string& name) {
    if (block) {
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
void JSONPrinter::flush() {
    if (block) {
        if (compact) {
            *result << block->dump();
        } else {
            *result << block->dump(2);
        }
        ofs.close();
        block = nullptr;
    }
}
