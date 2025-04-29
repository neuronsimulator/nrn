/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/float.hpp"
#include "ast/program.hpp"
#include "ast/string.hpp"
#include "symtab/symbol.hpp"
#include "symtab/symbol_table.hpp"

using namespace nmodl;
using namespace symtab;
using namespace syminfo;

//=============================================================================
// Symbol properties test
//=============================================================================

SCENARIO("Symbol properties can be added and converted to string") {
    NmodlType prop1{NmodlType::empty};
    NmodlType prop2 = NmodlType::local_var;
    NmodlType prop3 = NmodlType::global_var;

    GIVEN("A empty property") {
        WHEN("converted to string") {
            THEN("returns empty string") {
                REQUIRE(to_string(prop1).empty());
            }
        }
        WHEN("checked for property") {
            THEN("doesn't have any property") {
                REQUIRE_FALSE(has_property(prop1, NmodlType::local_var));
            }
        }
        WHEN("adding another empty property") {
            NmodlType result = prop1 | prop1;  // NOLINT(misc-redundant-expression)
            THEN("to_string still returns empty string") {
                REQUIRE(to_string(result).empty());
            }
        }
        WHEN("added some other property") {
            NmodlType result = prop1 | prop2;
            THEN("to_string returns added property") {
                REQUIRE(to_string(result) == "local");
            }
            WHEN("checked for property") {
                THEN("has required property") {
                    REQUIRE(has_property(result, NmodlType::local_var) == true);
                }
            }
        }
        WHEN("added multiple properties") {
            NmodlType result = prop1 | prop2 | prop3;
            result |= NmodlType::write_ion_var;
            THEN("to_string returns all added properties") {
                REQUIRE_THAT(to_string(result), Catch::Matchers::ContainsSubstring("local"));
                REQUIRE_THAT(to_string(result), Catch::Matchers::ContainsSubstring("global"));
                REQUIRE_THAT(to_string(result), Catch::Matchers::ContainsSubstring("write_ion"));
            }
            WHEN("checked for property") {
                THEN("has all added properties") {
                    REQUIRE(has_property(result, NmodlType::local_var) == true);
                    REQUIRE(has_property(result, NmodlType::global_var) == true);
                    REQUIRE(has_property(result, NmodlType::write_ion_var) == true);
                    REQUIRE_FALSE(has_property(result, NmodlType::read_ion_var));
                }
            }
        }
        WHEN("properties manipulated with bitwise operators") {
            THEN("& applied correctly") {
                NmodlType result1 = prop2 & prop2;  // NOLINT(misc-redundant-expression)
                NmodlType result2 = prop1 & prop2;
                NmodlType result3 = prop1 & prop2 & prop3;
                REQUIRE(to_string(result1) == "local");
                REQUIRE(to_string_vector(result2).empty());
                REQUIRE(result2 == NmodlType::empty);
                REQUIRE(result3 == NmodlType::empty);
            }
        }
    }
}

//=============================================================================
// Symbol test
//=============================================================================

SCENARIO("Multiple properties can be added to Symbol") {
    NmodlType property1 = NmodlType::argument;
    NmodlType property2 = NmodlType::range_var;
    NmodlType property3 = NmodlType::discrete_block;
    GIVEN("A symbol") {
        ModToken token(true);
        Symbol symbol("alpha", token);
        WHEN("added external property") {
            symbol.add_property(NmodlType::extern_neuron_variable);
            THEN("symbol becomes external") {
                REQUIRE(symbol.is_external_variable() == true);
            }
        }
        WHEN("added multiple properties to symbol") {
            symbol.add_property(property1);
            symbol.add_property(property2);
            THEN("symbol has multiple properties") {
                REQUIRE(symbol.has_any_property(property1) == true);

                REQUIRE(symbol.has_any_property(property3) == false);

                symbol.add_property(property3);
                REQUIRE(symbol.has_any_property(property3) == true);

                auto property = property1 | property2;
                REQUIRE(symbol.has_all_properties(property) == true);

                property |= property3;
                REQUIRE(symbol.has_all_properties(property) == true);

                property = property2 | property3;
                REQUIRE(symbol.has_all_properties(property) == true);

                property |= NmodlType::to_solve;
                REQUIRE(symbol.has_all_properties(property) == false);
            }
        }
        WHEN("remove properties from symbol") {
            symbol.add_property(property1);
            symbol.add_property(property2);
            THEN("remove property that exists") {
                REQUIRE(symbol.has_any_property(property2) == true);
                symbol.remove_property(property2);
                REQUIRE(symbol.has_any_property(property2) == false);
            }
            THEN("remove property that doesn't exist") {
                REQUIRE(symbol.has_any_property(property3) == false);
                symbol.remove_property(property3);
                REQUIRE(symbol.has_any_property(property3) == false);
                auto properties = property1 | property2;
                REQUIRE(symbol.has_all_properties(properties) == true);
            }
        }
        WHEN("combined properties") {
            NmodlType property = NmodlType::factor_def | NmodlType::global_var;
            THEN("symbol has union of all properties") {
                REQUIRE(symbol.has_any_property(property) == false);
                symbol.add_properties(property);
                REQUIRE(symbol.has_any_property(property) == true);
                property |= symbol.get_properties();
                REQUIRE(symbol.get_properties() == property);
            }
        }
    }
}

//=============================================================================
// Symbol table test
//=============================================================================

SCENARIO("Symbol table allows operations like insert, lookup") {
    GIVEN("A global SymbolTable") {
        auto program = std::make_shared<ast::Program>();
        auto table = std::make_shared<SymbolTable>("Na", program.get(), true);
        auto symbol = std::make_shared<Symbol>("alpha");

        WHEN("checked methods and member variables") {
            THEN("all members are initialized") {
                REQUIRE(table->under_global_scope());
                REQUIRE_THAT(table->name(), Catch::Matchers::ContainsSubstring("Na"));
                REQUIRE_THAT(table->get_parent_table_name(),
                             Catch::Matchers::ContainsSubstring("None"));
                REQUIRE_THAT(table->position(), Catch::Matchers::ContainsSubstring("UNKNOWN"));
            }
        }
        WHEN("insert symbol") {
            table->insert(symbol);
            THEN("table size increases") {
                REQUIRE(table->symbol_count() == 1);
            }
            THEN("lookup returns an inserted symbol") {
                REQUIRE(table->lookup("alpha") != nullptr);
                REQUIRE(table->lookup("beta") == nullptr);
            }
            WHEN("re-inserting the same symbol") {
                THEN("throws an exception") {
                    REQUIRE_THROWS_WITH(table->insert(symbol),
                                        Catch::Matchers::ContainsSubstring("re-insert"));
                }
            }
            WHEN("inserting another symbol") {
                auto next_symbol = std::make_shared<Symbol>("beta");
                table->insert(next_symbol);
                THEN("symbol gets added and table size increases") {
                    REQUIRE(table->symbol_count() == 2);
                    REQUIRE(table->lookup("beta") != nullptr);
                }
            }
        }
        WHEN("checked for global variables") {
            table->insert(symbol);
            auto variables = table->get_variables_with_properties(NmodlType::range_var);
            THEN("table doesn't have any global variables") {
                REQUIRE(variables.empty());
                WHEN("added global symbol") {
                    auto next_symbol = std::make_shared<Symbol>("gamma");
                    next_symbol->add_property(NmodlType::assigned_definition);
                    table->insert(next_symbol);
                    auto variables = table->get_variables_with_properties(
                        NmodlType::assigned_definition);
                    THEN("table has global variable") {
                        REQUIRE(variables.size() == 1);
                    }
                }
            }
        }
        WHEN("added another symbol table as children") {
            table->insert(symbol);
            auto next_program = std::make_shared<ast::Program>();
            auto next_table = std::make_shared<SymbolTable>("Ca", next_program.get(), true);
            next_table->set_parent_table(table.get());
            THEN("children symbol table can lookup into parent table scope") {
                REQUIRE(next_table->lookup("alpha") == nullptr);
                REQUIRE(next_table->lookup_in_scope("alpha") != nullptr);
            }
            THEN("children can figure if it is in global scope or not") {
                REQUIRE(next_table->global_scope() == table->global_scope());
            }
        }
        WHEN("pretty-printing a symbol table to a stream") {
            std::ostringstream oss;
            table->print(oss, 0);
            auto text = oss.str();
            THEN("nothing is written when the table is empty") {
                REQUIRE(text.empty());
            }
            table->insert(symbol);
            table->print(oss, 0);
            text = oss.str();
            THEN("the symbol present in the table can be found in the written string") {
                REQUIRE(text.find(symbol->get_name()) != std::string::npos);
            }
        }
        WHEN("creating a clone of symbol table") {
            const std::unique_ptr<SymbolTable> clone(table->clone());
            THEN("clone has the same name") {
                REQUIRE(clone->name() == table->name());
            }
        }
        WHEN("query for symbol with and without properties") {
            auto symbol1 = std::make_shared<Symbol>("alpha");
            auto symbol2 = std::make_shared<Symbol>("beta");
            auto symbol3 = std::make_shared<Symbol>("gamma");
            auto symbol4 = std::make_shared<Symbol>("delta");

            symbol1->add_property(NmodlType::range_var | NmodlType::param_assign);
            symbol2->add_property(NmodlType::range_var | NmodlType::param_assign |
                                  NmodlType::state_var);
            symbol3->add_property(NmodlType::range_var | NmodlType::assigned_definition |
                                  NmodlType::pointer_var);
            symbol4->add_property(NmodlType::range_var);

            table->insert(symbol1);
            table->insert(symbol2);
            table->insert(symbol3);
            table->insert(symbol4);

            auto result = table->get_variables_with_properties(NmodlType::range_var);
            REQUIRE(result.size() == 4);

            result = table->get_variables_with_properties(NmodlType::range_var |
                                                          NmodlType::pointer_var);
            REQUIRE(result.size() == 4);

            auto with = NmodlType::range_var | NmodlType::param_assign;
            auto without = NmodlType::state_var | NmodlType::pointer_var;
            result = table->get_variables(with, without);
            REQUIRE(result.size() == 1);
            REQUIRE(result[0]->get_name() == "alpha");


            with = NmodlType::range_var;
            without = NmodlType::param_assign | NmodlType::assigned_definition;
            result = table->get_variables(with, without);
            REQUIRE(result.size() == 1);
            REQUIRE(result[0]->get_name() == "delta");

            with = NmodlType::range_var;
            without = NmodlType::range_var;
            result = table->get_variables(with, without);
            REQUIRE(result.empty());
        }
    }
}

//=============================================================================
// Model symbol table test
//=============================================================================

SCENARIO("Global symbol table (ModelSymbol) allows scope based operations") {
    GIVEN("A Model symbolTable") {
        ModelSymbolTable mod_symtab;

        auto program = std::make_shared<ast::Program>();
        auto symbol1 = std::make_shared<Symbol>("alpha");
        auto symbol2 = std::make_shared<Symbol>("alpha");
        auto symbol3 = std::make_shared<Symbol>("alpha");

        symbol1->add_property(NmodlType::param_assign);
        symbol2->add_property(NmodlType::range_var);
        symbol3->add_property(NmodlType::range_var);

        SymbolTable* old_symtab = nullptr;

        WHEN("trying to exit scope without entering") {
            THEN("throws an exception") {
                REQUIRE_THROWS_WITH(mod_symtab.leave_scope(),
                                    Catch::Matchers::ContainsSubstring("without entering"));
            }
        }
        WHEN("trying to enter scope without valid node") {
            THEN("throws an exception") {
                REQUIRE_THROWS_WITH(mod_symtab.enter_scope("scope", nullptr, true, old_symtab),
                                    Catch::Matchers::ContainsSubstring("empty node"));
            }
        }
        WHEN("trying to insert without entering scope") {
            THEN("throws an exception") {
                auto symbol = std::make_shared<Symbol>("alpha");
                REQUIRE_THROWS_WITH(mod_symtab.insert(symbol),
                                    Catch::Matchers::ContainsSubstring("Can not insert"));
            }
        }
        WHEN("enter scope multiple times") {
            auto program1 = std::make_shared<ast::Program>();
            auto program2 = std::make_shared<ast::Program>();
            mod_symtab.enter_scope("scope1", program1.get(), false, old_symtab);
            mod_symtab.enter_scope("scope2", program2.get(), false, old_symtab);
            THEN("can leave scope multiple times") {
                mod_symtab.leave_scope();
                mod_symtab.leave_scope();
            }
        }
        WHEN("added same symbol with different properties in global scope") {
            mod_symtab.enter_scope("scope", program.get(), true, old_symtab);
            mod_symtab.insert(symbol1);
            mod_symtab.insert(symbol2);
            THEN("only one symbol gets added with combined properties") {
                auto symbol = mod_symtab.lookup("alpha");
                auto properties = NmodlType::param_assign | NmodlType::range_var;
                REQUIRE(symbol->get_properties() == properties);
            }
        }
        WHEN("added same symbol with existing property") {
            mod_symtab.enter_scope("scope", program.get(), true, old_symtab);
            mod_symtab.insert(symbol1);
            mod_symtab.insert(symbol2);
            THEN("throws an exception") {
                REQUIRE_THROWS_WITH(mod_symtab.insert(symbol3),
                                    Catch::Matchers::ContainsSubstring("Re-declaration"));
            }
        }
        WHEN("added same symbol in children scope") {
            mod_symtab.enter_scope("scope1", program.get(), true, old_symtab);
            mod_symtab.insert(symbol2);
            THEN("it's ok, just get overshadow warning") {
                mod_symtab.enter_scope("scope2", program.get(), false, old_symtab);
                mod_symtab.insert(symbol3);
                ///\todo : not sure how to capture std::cout
            }
        }
    }
}

//=============================================================================
// Symbol class tests
//=============================================================================

SCENARIO("Symbol class allows manipulation") {
    GIVEN("A symbol can have several nodes") {
        auto st = std::make_shared<ast::String>("node1");
        auto fl = std::make_shared<ast::Float>("1.1");
        Symbol symbol1("alpha");
        symbol1.add_node(st.get());
        symbol1.add_node(fl.get());

        Symbol symbol2("beta");

        WHEN("trying to get name") {
            THEN("it works") {
                REQUIRE(symbol1.get_name() == "alpha");
                REQUIRE(symbol2.get_name() == "beta");
            }
        }

        WHEN("trying to get all nodes") {
            THEN("it works") {
                REQUIRE(symbol1.get_nodes().size() == 2);
                REQUIRE(symbol2.get_nodes().empty());
            }
        }

        WHEN("trying to get specific node") {
            auto nodes = symbol1.get_nodes_by_type({ast::AstNodeType::STRING});

            THEN("it works") {
                REQUIRE(nodes.size() == 1);
                REQUIRE(nodes.front()->is_string());
                REQUIRE(symbol2.get_nodes_by_type({ast::AstNodeType::STRING}).empty());
            }
        }
        WHEN("read and write counters works") {
            symbol1.read();
            symbol1.read();
            symbol1.write();

            THEN("it works") {
                REQUIRE(symbol1.get_read_count() == 2);
                REQUIRE(symbol1.get_write_count() == 1);
                REQUIRE(symbol2.get_read_count() == 0);
                REQUIRE(symbol2.get_write_count() == 0);
            }
        }

        WHEN("renaming a symbol") {
            symbol2.set_name("gamma");
            THEN("get_name return the new name") {
                REQUIRE(symbol2.get_name() == "gamma");
                REQUIRE(symbol2.get_original_name() == "beta");
            }
            symbol2.set_original_name("gamma");
            THEN("get_original_name return the new name") {
                REQUIRE(symbol2.get_original_name() == "gamma");
            }
        }

        WHEN("set as array") {
            symbol1.set_as_array(15);
            THEN("recognized as an array") {
                REQUIRE(symbol1.get_length() == 15);
                REQUIRE(symbol1.is_array());

                REQUIRE(symbol2.get_length() == 1);
                REQUIRE(!symbol2.is_array());
            }
        }
    }
}
