/*
  Copyright (c) 2012 Joel Nises

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef DEEPNESS_IMMUTABLE_MAP_HPP
#define DEEPNESS_IMMUTABLE_MAP_HPP

#include <memory>
#include <functional>
#include <vector>
#include <exception>
#include <cassert>
#include <cstdint>

namespace deepness
{
	class key_error: public std::exception
	{
	public:
		key_error(std::string message = "")
			:m_message(std::move(message))
		{}

		const char *what() const throw() override
		{
			return m_message.c_str();
		}
	private:
		std::string m_message;
	};

    template<class Key,
             class T,
             class Hash = std::hash<Key>,
             class Pred = std::equal_to<Key> >
    class immutable_map
    {
    public:
        typedef std::pair<const Key, T> value_type;
        typedef Key key_type;
        typedef T mapped_type;
        typedef Hash hasher;
        typedef Pred key_equals;

    private:
        struct node_type;
        typedef std::shared_ptr<node_type> shared_node_type;
        typedef std::shared_ptr<value_type> shared_value_type;
        struct node_type
        {
            node_type()
                :population(0)
            {}

            node_type(const node_type &other)
                :population(other.population)
                ,values(other.values)
            {
                size_t num_children = get_num_children();
                if(num_children)
                {
                    children.reset(new shared_node_type[num_children]);
                    std::copy(other.children.get(), other.children.get() + num_children, children.get());
                }
            }

            size_t get_num_children()
            {
                return popcnt(population);
            }

            shared_node_type &get_child(size_t child)
            {
                return children[popcnt(population & ((1 << child) - 1))];
            }

            bool has_child(uint32_t hash)
            {
                assert((hash & ~0x1f) == 0);
                return (population >> hash) & 1;
            }

            // TODO this is just a naive implementation, redo it to optimize
            uint32_t population; //!< a 0 population indicates a leaf
            std::unique_ptr<shared_node_type[]> children;
            std::vector<shared_value_type> values;
        };

    public:
        immutable_map()
            :m_root(std::make_shared<node_type>())
        {}

        /*!
          \returns a new immutable_map with val.first mapped to val.second
        */
        immutable_map set(value_type val) const
        {
            uint32_t hash = static_cast<uint32_t>(hasher()(val.first));
            return immutable_map(set(std::move(val), hash, 0, m_root));
        }

        immutable_map set(key_type key, mapped_type value) const
        {
            return set(std::make_pair(std::move(key), std::move(value)));
        }

        /*!
          Note that this returns a reference to mapped_type. Make sure
          mapped_type is immutable somehow if you want to make sure
          the values in the map doesn't change.
          \throws key_error if the map doesn't contain the specified key.
        */
        mapped_type &operator[](const key_type &key) const
        {
            uint32_t hash = static_cast<uint32_t>(hasher()(key));
            return get(key, hash, m_root);
        }

        /*!
          \throws key_error if the map doesn't contain the specified key.
        */
        immutable_map erase(const key_type &key) const
        {
            uint32_t hash = static_cast<uint32_t>(hasher()(key));            
            return immutable_map(erase(key, hash, 0, m_root));
        }

    private:
        immutable_map(shared_node_type node)
            :m_root(std::move(node))
        {}

        static mapped_type &get(const key_type &val, uint32_t hash, const shared_node_type &node)
        {
            if(node->population)
            {
                uint32_t partial = hash & 0x1f;
                if((node->population >> partial) & 1)
                {
                    return get(val, hash >> 5, node->get_child(partial));
                }
                else
                {
                    // no node with a matching key
                    throw key_error();
                }
            }
            else
            {
                // we have found a leaf
                for(auto it = node->values.begin(); it != node->values.end(); ++it)
                {
                    if(key_equals()((*it)->first, val))
                    {
                        return (*it)->second;
                    }
                }
                // the leaf didn't contain our value
                throw key_error();
            }
        }

        static shared_node_type set(value_type val, uint32_t hash, int level, const shared_node_type &node)
        {
            uint32_t partial = hash & 0x1f;
            if(node->population)
            {
                if(node->has_child(partial))
                {
                    auto newnode = std::make_shared<node_type>(*node);
                    newnode->get_child(partial) = set(std::move(val), hash >> 5, level + 1, node->get_child(partial));
                    return std::move(newnode);
                }
                else
                {
                    // no node with a matching key, create a new one
                    size_t num_children = node->get_num_children();
                    auto newnode = std::make_shared<node_type>();
                    uint32_t partialbit = 1 << partial;
                    newnode->population = node->population | partialbit;
                    assert(num_children == newnode->get_num_children() - 1);
                    newnode->children.reset(new shared_node_type[num_children + 1]);
                    uint32_t before_popcnt = popcnt(node->population & (partialbit - 1));
                    uint32_t after_popcnt = popcnt(node->population & ~(partialbit | (partialbit - 1)));
                    std::copy(node->children.get(), node->children.get() + before_popcnt, newnode->children.get());
                    newnode->children[before_popcnt] = std::make_shared<node_type>();
                    newnode->children[before_popcnt]->values.push_back(std::make_shared<value_type>(std::move(val)));
                    std::copy(node->children.get() + before_popcnt, node->children.get() + num_children, newnode->children.get() + before_popcnt + 1);
                    return std::move(newnode);
                }
            }
            else
            {
                if(node->values.size())
                {
                    for(size_t i = 0; i < node->values.size(); ++i)
                    {
                        if(key_equals()(node->values[i]->first, val.first))
                        {
                            // the key already exists in the map, replace it
                            auto newnode = std::make_shared<node_type>(*node);
                            newnode->values[i] = std::make_shared<value_type>(std::move(val));
                            return std::move(newnode);
                        }
                    }

                    // the found leaf doesn't match the key
                    if(level > 6)
                    {
                        auto newnode = std::make_shared<node_type>(*node);
                        newnode->values.push_back(std::make_shared<value_type>(std::move(val)));
                        return std::move(newnode);
                    }
                    else
                    {
                        auto splitnode = std::make_shared<node_type>();
                        uint32_t oldhash = static_cast<uint32_t>(hasher()(node->values.front()->first));
                        uint32_t shifted_oldhash = oldhash >> (5 * level);
                        uint32_t shifted_partial = shifted_oldhash & 0x1f;
                        splitnode->population = 1 << shifted_partial;
                        splitnode->children.reset(new shared_node_type[1]);
                        splitnode->children[0] = std::make_shared<node_type>(*node);
                        return set(std::move(val), hash, level, splitnode);
                    }
                }
                else
                {
                    // this is the root node
                    auto newnode = std::make_shared<node_type>();
                    newnode->population = 1 << partial;
                    newnode->children.reset(new shared_node_type[1]);
                    newnode->children[0] = std::make_shared<node_type>();
                    newnode->children[0]->values.push_back(std::make_shared<value_type>(std::move(val)));
                    return std::move(newnode);
                }
            }
        }

        static shared_node_type erase(const key_type &key, uint32_t hash, int level, const shared_node_type &node)
        {
            uint32_t partial = hash & 0x1f;
            if(node->population)
            {
                if(node->has_child(partial))
                {
                    auto newchild = erase(key, hash >> 5, level + 1, node->get_child(partial));
                    if(newchild)
                    {
                        auto newnode = std::make_shared<node_type>(*node);
                        newnode->get_child(partial) = newchild;
                        return std::move(newnode);
                    }
                    else
                    {
                        uint32_t other_children = node->population & ~(1 << partial);
                        size_t node_num_children = node->get_num_children();
                        // if there is only one child, this must be the root node, treat it like a node with more than two children
                        if(node_num_children == 2)
                        {
                            // if there is only one child left after the erase, return it
                            return node->children[popcnt(node->population & (other_children - 1))];
                        }
                        else
                        {
                            auto newnode = std::make_shared<node_type>();
                            newnode->population = other_children;
                            newnode->children.reset(new shared_node_type[node_num_children - 1]);
                            for(size_t srcit = 0, dstit = 0; srcit < node_num_children; ++srcit)
                            {
                                if(srcit != partial)
                                {
                                    newnode->children[dstit] = node->children[srcit];
                                    ++dstit;
                                }
                            }
                            return std::move(newnode);
                        }
                    }
                }
                else
                {
                    // no node with a matching key, this is an error
                    throw key_error();
                }
            }
            else
            {
                for(size_t i = 0; i < node->values.size(); ++i)
                {
                    if(key_equals()(node->values[i]->first, key))
                    {
                        // create a new node without the erased value
                        if(node->values.size() > 1)
                        {
                            auto newnode = std::make_shared<node_type>();
                            newnode->values.reserve(node->values.size() - 1);
                            for(size_t j = 0; j < node->values.size(); ++j)
                            {
                                if(i != j)
                                    newnode->values.push_back(node->values[j]);
                            }
                            return std::move(newnode);
                        }
                        // return null if we erased the last value
                        return shared_node_type();
                    }
                }

                throw key_error();
            }
        }

        static uint32_t popcnt(uint32_t val)
        {
            // TODO create compile time check for hardware popcnt instruction
            val = val - ((val >> 1) & 0x55555555);
            val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
            return (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        }

        shared_node_type m_root;
    };
}

#endif
