/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "printer/json_printer.hpp"
#include "utils/logger.hpp"


namespace nmodl {
namespace printer {

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
void JSONPrinter::add_node(std::string value, const std::string& key) {
    if (!block) {
        throw std::logic_error{"Block not initialized (push_block missing?)"};
    }

    json j;
    j[key] = std::move(value);
    block->front().push_back(std::move(j));
}

/// Add property to the block which is added last
void JSONPrinter::add_block_property(std::string const& name, const std::string& value) {
    if (block == nullptr) {
        logger->warn("JSONPrinter : can't add property without block");
        return;
    }
    (*block)[name] = value;
}

/// Add new json object (typically start of new block)
/// name here is type of new block encountered
void JSONPrinter::push_block(const std::string& value, const std::string& key) {
    if (block) {
        stack.push(block);
    }

    json j;
    if (expand) {
        j[key] = value;
        j[child_key] = json::array();
    } else {
        j[value] = json::array();
    }
    block = std::shared_ptr<json>(new json(j));
}

/// We finished processing a block, add processed block to it's parent block
void JSONPrinter::pop_block() {
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

}  // namespace printer
}  // namespace nmodl
