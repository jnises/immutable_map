#ifndef DEEPNESS_IMMUTABLE_MAP_HPP
#define DEEPNESS_IMMUTABLE_MAP_HPP

#include <memory>
#include <functional>
#include <vector>

namespace deepness
{
    template<class Key,
             class T,
             class Hash = std::hash<Key>,
             class Pred = std::equal_to<Key>,
             class Alloc = std::allocator< std::pair<const Key,T> > >
    class immutable_map
    {
    public:
        typedef std::pair<const Key, T> value_type;
        typedef Key key_type;
        typedef T mapped_type;
        typedef Hash hasher;
        typedef Pred key_equals;
        typedef Alloc allocator_type;

    private:
        struct node_type;
        typedef std::shared_ptr<node_type> shared_node_type;
        typedef std::vector<std::pair<shared_node_type, size_t> > node_path_type;
        struct node_type
        {
            // TODO this is just a naive implementation, redo it to optimize
            uint32_t population; //!< a 0 population indicates a leaf
            std::unique_ptr<shared_node_type[]> children;
            std::vector<value_type> values;
        };

    public:        
        class const_iterator
        {
            friend immutable_map;
        public:
            value_type operator*() const
            {
                // TODO test for end?
                return nodepath.back().first->values[nodepath.back().second];
            }

            bool operator==(const const_iterator &other)
            {
                return other.nodepath == nodepath;
            }
        private:
            const_iterator(node_path_type path)
                :nodepath(std::move(path))
            {}

            node_path_type nodepath;
        };
            
        immutable_map()
        {
            root->population = 0;
        }
        
        immutable_map insert(const value_type &val) const;
        const_iterator find(const key_type &val) const
        {
            uint32_t hash = static_cast<uint32_t>(hasher()(val));
            return const_iterator(find(val, hash, root));
        }

        const_iterator end() const
        {
            return const_iterator(node_path_type());
        }

    private:
        static node_path_type find(const key_type &val, uint32_t hash, const shared_node_type &node)
        {
            if(node->population)
            {
                uint32_t partial = hash & 0x1f;
                if((node->population >> partial) & 1)
                {
                    return find(val, hash >> 5, node->children[partial]);
                }
                else
                {
                    // no node with a matching key
                    return node_path_type();
                }
            }
            else
            {
                // we have found a leaf
                for(auto it = node->values.begin(); it != node->values.end(); ++it)
                {
                    if(key_equals(it->first, val))
                    {
                        node_path_type vec(1);
                        vec.push_back(std::make_pair(node, static_cast<size_t>(std::distance(node->values.begin(), it))));
                        return std::move(vec);
                    }
                    else
                    {
                        // the leaf didn't contain our value
                        return node_path_type();
                    }
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

        shared_node_type root;
    };
}

#endif
