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
                size_t num_children = popcnt(population);
                if(num_children)
                {
                    children.reset(new shared_node_type[num_children]);
                    std::copy(other.children.get(), other.children.get() + num_children, children.get());
                }
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
          \throws key_error if the map doesn't contain the specified key.
        */
        mapped_type &operator[](const key_type &key) const
        {
            uint32_t hash = static_cast<uint32_t>(hasher()(key));
            return get(key, hash, m_root);
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
                    return get(val, hash >> 5, node->children[partial]);
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
                    else
                    {
                        // the leaf didn't contain our value
                        throw key_error();
                    }
                }
            }
        }

        static shared_node_type set(value_type val, uint32_t hash, int level, const shared_node_type &node)
        {
            if(node->population)
            {
                uint32_t partial = hash & 0x1f;
                if((node->population >> partial) & 1)
                {
                    auto newnode = std::make_shared<node_type>(*node);                
                    newnode->children[partial] = set(std::move(val), hash >> 5, level + 1, node->children[partial]);
                    return std::move(newnode);
                }
                else
                { 
                    // no node with a matching key, create a new one
                    size_t num_children = popcnt(node->population);
                    auto newnode = std::make_shared<node_type>();
                    uint32_t partialbit = 1 << partial;
                    newnode->population = node->population & partialbit;
                    assert(num_children == popcnt(newnode->population) - 1);
                    newnode->children.reset(new shared_node_type[num_children + 1]);
                    uint32_t before_popcnt = popcnt(node->population & (partialbit - 1));
                    uint32_t after_popcnt = popcnt(node->population & ~(partialbit | (partialbit - 1)));
                    std::copy(node->children.get(), node->children.get() + before_popcnt, newnode->children.get());
                    newnode->children[partial] = std::make_shared<node_type>();
                    newnode->children[partial]->values.push_back(std::make_shared<value_type>(std::move(val)));
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
						return set(std::move(val), hash >> 5, level + 1, splitnode);
					}
				}
				else
				{
					// this is the root node
                    auto newnode = std::make_shared<node_type>();
                    newnode->values.push_back(std::make_shared<value_type>(std::move(val)));
                    return std::move(newnode);
				}
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
