#ifndef GRAPH_COLORS_HPP
#define GRAPH_COLORS_HPP

template<class mcolor_t>
struct ColorsGraph { 
  
  template<class conf_t>
  ColorsGraph(conf_t const & cfg); 

  std::set<mcolor_t> split_color_on_tc_color(mcolor_t const & color, size_t number_splits) const;
  std::set<mcolor_t> split_color_on_vtc_color(mcolor_t const & color) const;

  inline mcolor_t const & get_complement_color(mcolor_t const & color) { 
    assert(color.is_one_to_one_match()); 
    if (compliment_colors.count(color) == 0) {  
      mcolor_t const & temp = compute_complement_color(color);  
      compliment_colors.insert(std::make_pair(color, temp));
      compliment_colors.insert(std::make_pair(temp, color));
    }  
    return compliment_colors.find(color)->second;
  } 
  
  inline bool is_T_consistent_color(mcolor_t const & color) const { 
    return (T_consistent_colors.count(color) > 0);
  } 

  inline bool is_vec_T_consistent_color(mcolor_t const & color) const {
    return (vec_T_consistent_colors.count(color) > 0);
  }

  DECLARE_GETTER(mcolor_t const &, complete_color, complete_color)
  DECLARE_GETTER(mcolor_t const &, remove_color, root_color)
  DECLARE_DELEGATE_CONST_METHOD( size_t, vec_T_consistent_colors, count_vec_T_consitent_color, size )

  typedef typename std::set<mcolor_t>::const_iterator citer; 
  DECLARE_CONST_ITERATOR( citer, T_consistent_colors, cbegin_T_consistent_color, cbegin )  
  DECLARE_CONST_ITERATOR( citer, T_consistent_colors, cend_T_consistent_color, cend )
  DECLARE_CONST_ITERATOR( citer, vec_T_consistent_colors, cbegin_vec_T_consistent_color, cbegin )  
  DECLARE_CONST_ITERATOR( citer, vec_T_consistent_colors, cend_vec_T_consistent_color, cend )
  
private: 
  inline mcolor_t compute_complement_color(mcolor_t const & color) const {
    mcolor_t answer(complete_color, color, mcolor_t::Difference); 
    return answer;
  }

protected:
  mcolor_t complete_color;
  mcolor_t remove_color; 
  std::map<mcolor_t, mcolor_t> compliment_colors;
  std::set<mcolor_t> T_consistent_colors;
  std::set<mcolor_t> vec_T_consistent_colors;
}; 

template<class mcolor_t>
template<class conf_t>
ColorsGraph<mcolor_t>::ColorsGraph(conf_t const & cfg) 
{
  for (size_t i = 0; i < cfg.get_count_genomes(); ++i) {
    complete_color.insert(i);
    vec_T_consistent_colors.insert(mcolor_t(i));
  }

  std::set<mcolor_t> nodes_color;
  for (auto it = cfg.cbegin_trees(); it != cfg.cend_trees(); ++it) {
    auto const & tree_color = it->build_vec_T_consistent_colors();
    nodes_color.insert(tree_color.cbegin(), tree_color.cend());
  }
  nodes_color.erase(complete_color);

  if (cfg.get_target().empty()) { 
    for (auto const & color : nodes_color) {
      auto const & compl_color = compute_complement_color(color);
      if (nodes_color.find(compl_color) != nodes_color.end()) {
        if (color.size() > compl_color.size()) { 
          remove_color = color;
        } else { 
          remove_color = compl_color; 
        } 
        nodes_color.erase(remove_color);
        break; 
      } 
    } 
  }

  vec_T_consistent_colors = nodes_color;
  if (!cfg.get_target().empty()) { 
    remove_color = cfg.get_target();
    vec_T_consistent_colors.erase(cfg.get_target());
  } 

  //check consistency
  for (auto id = vec_T_consistent_colors.cbegin(); id != vec_T_consistent_colors.cend(); ++id) {
    for(auto jd = id; jd != vec_T_consistent_colors.end(); ++jd) {
      mcolor_t color(*id, *jd, mcolor_t::Intersection);
      if (!color.empty() && color.size() != id->size() && color.size() != jd->size()) {
      	vec_T_consistent_colors.erase(jd++);
      	--jd;
      }
    }
  }
   
  for (auto const & vtc : vec_T_consistent_colors) {
    T_consistent_colors.insert(vtc);
    T_consistent_colors.insert(compute_complement_color(vtc));
  }
}

template<class mcolor_t>
std::set<mcolor_t> ColorsGraph<mcolor_t>::split_color_on_tc_color(mcolor_t const & color, size_t number_splits) const {
  if (is_T_consistent_color(color) || (number_splits == 1)) {
    return std::set<mcolor_t>({color}); 
  } else {  
    std::set<mcolor_t> answer;    
    /*if (hashing_split_colors.count(std::make_pair(color, number_of_splits)) != 0) {
      answer = hashing_split_colors.find(std::make_pair(color, number_of_splits))->second;
    } else {   */
    utility::equivalence<size_t> equiv;
    std::for_each(color.cbegin(), color.cend(), [&] (std::pair<size_t, size_t> const & col) -> void {
      equiv.addrel(col.first, col.first);
    }); 

    for (auto const & tc: T_consistent_colors) { 
      mcolor_t inter_color(tc, color, mcolor_t::Intersection);
      if (inter_color.size() >= 2 && inter_color.size() == tc.size()) { 
        std::for_each(inter_color.cbegin(), inter_color.cend(), [&] (std::pair<size_t, size_t> const & col) -> void {
          equiv.addrel(col.first, inter_color.cbegin()->first);
        });
      }
    }
    
    equiv.update();
    std::map<size_t, mcolor_t> const & classes = equiv.get_eclasses<mcolor_t>(); 
    //std::cerr << "color " << genome_match::mcolor_to_name(color) << std::endl;
    for(auto const & col : classes) {
      answer.insert(col.second);
    }

    if (answer.size() > number_splits) { 
      answer.clear(); 
      answer.insert(color);   
    }

    return answer;
  }    
}

template<class mcolor_t>
std::set<mcolor_t> ColorsGraph<mcolor_t>::split_color_on_vtc_color(mcolor_t const & color) const {
  if (is_vec_T_consistent_color(color)) {
    return std::set<mcolor_t>({color}); 
  } else {  
    std::set<mcolor_t> answer;    

    utility::equivalence<size_t> equiv;
    std::for_each(color.cbegin(), color.cend(), [&] (std::pair<size_t, size_t> const & col) -> void {
      equiv.addrel(col.first, col.first);
    }); 

    for (auto const & vtc: vec_T_consistent_colors) { 
      mcolor_t inter_color(vtc, color, mcolor_t::Intersection);
      if (inter_color.size() >= 2 && inter_color.size() == vtc.size()) {
        std::for_each(inter_color.cbegin(), inter_color.cend(), [&] (std::pair<size_t, size_t> const & col) -> void {
          equiv.addrel(col.first, inter_color.cbegin()->first);
        });
      }
    }

    equiv.update();
    std::map<size_t, mcolor_t> const & classes = equiv.get_eclasses<mcolor_t>(); 
    //std::cerr << "color " << genome_match::mcolor_to_name(color) << std::endl;
    for(auto const & col : classes) {
      answer.insert(col.second);
    }

    return answer;
 
  }
}


#endif
