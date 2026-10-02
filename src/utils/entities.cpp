#include "std_include.hpp"
#include "map_export.hpp"

namespace utils
{
	// build all entities (includes brushmodel pointers "*")
	std::string entities::build_all()
	{
		std::string entity_string;

		for (auto& entity : this->entities_)
		{
			entity_string.append("{\n");

			for (auto& property : entity)
			{
				entity_string.push_back('"');
				entity_string.append(property.first);
				entity_string.append("\" \"");
				entity_string.append(property.second);
				entity_string.append("\"\n");
			}

			entity_string.append("}\n");
		}

		return entity_string;
	}

	// build all entities and fix brushmodels
	std::string entities::build_all_script_structs()
	{
		std::string entity_string;

		for (auto& entity : this->entities_)
		{
			std::string classname = entity["classname"];

			// if ent is a brushmodel/submodel
			if (!classname.empty() && utils::contains(classname, "script_struct"))
			{
				// start entity
				//entityString.append(utils::va("// entity %d\n", entityNum));
				entity_string.append("{\n");

				for (auto& property : entity)
				{
					entity_string.push_back('"');
					entity_string.append(property.first);
					entity_string.append("\" \"");
					entity_string.append(property.second);
					entity_string.append("\"\n");
				}

				// close entity
				entity_string.append("}\n");
			}
		}

		return entity_string;
	}

	// build all entities and fix brushmodels
	std::string entities::buildAll_FixBrushmodels(const std::vector<game::brushmodel_entity_s> &bmodel_list)
	{
		int entity_num = 1; // worldspawn is 0
		int submodel_num = 0;

		std::string basic_string;

		for (auto& entity : this->entities_)
		{
			std::string model = entity["model"];

			// if ent is a brushmodel/submodel
			if (!model.empty() && model[0] == '*')
			{
				// get the submodel index 
				const auto p_index = std::stoi(model.erase(0, 1));

				if (p_index > 0 && p_index < static_cast<int>(bmodel_list.size()))
				{
					if (bmodel_list[p_index].brushes.empty())
					{
						continue;
					}

					// start submodel
					basic_string.append(utils::va("// submodel %d\n", submodel_num));
					basic_string.append("{\n");

					// write submodel keys
					for (auto& property : entity)
					{
						// do not write model/origin keys
						if (property.first == "model" || property.first == "origin")
						{
							continue;
						}

						basic_string.push_back('"');
						basic_string.append(property.first);
						basic_string.append("\" \"");
						basic_string.append(property.second);
						basic_string.append("\"\n");
					}

					for (const auto& brush : bmodel_list[p_index].brushes)
					{
						basic_string.append("{\n");
						for (const auto& side : brush) basic_string.append(side);
						basic_string.append("}\n");
					}

					// close submodel
					basic_string.append("}\n");

					submodel_num++;
				}
			}

			else
			{
				// start entity
				basic_string.append(utils::va("// entity %d\n", entity_num));
				basic_string.append("{\n");

				for (auto& property : entity)
				{
					basic_string.push_back('"');
					basic_string.append(property.first);
					basic_string.append("\" \"");
					basic_string.append(property.second);
					basic_string.append("\"\n");
				}

				// close entity
				basic_string.append("}\n");

				entity_num++;
			}
		}

		return basic_string;
	}

	// build all selected entities and fix brushmodels
	std::string entities::buildSelection_FixBrushmodels(const game::boundingbox_s* sbox, const std::vector<game::brushmodel_entity_s>& bmodel_list)
	{
		int entity_num = 1; // worldspawn is 0
		int submodel_num = 0;

		std::string entity_string;

		for (auto& entity : this->entities_)
		{
			std::string model = entity["model"];
			std::string origin = entity["origin"];

			// if ent is a brushmodel/submodel
			if (!model.empty() && model[0] == '*')
			{
				// get the submodel index 
				const auto p_index = std::stoi(model.erase(0, 1));

				if (p_index > 0 && p_index < static_cast<int>(bmodel_list.size()))
				{
					if (bmodel_list[p_index].brushes.empty())
					{
						continue;
					}

					// start submodel
					entity_string.append(utils::va("// submodel %d\n", submodel_num));
					entity_string.append("{\n");

					// write submodel keys
					for (auto& property : entity)
					{
						// do not write model/origin keys
						if (property.first == "model" || property.first == "origin")
						{
							continue;
						}

						entity_string.push_back('"');
						entity_string.append(property.first);
						entity_string.append("\" \"");
						entity_string.append(property.second);
						entity_string.append("\"\n");
					}

					for (const auto& brush : bmodel_list[p_index].brushes)
					{
						entity_string.append("{\n");
						for (const auto& side : brush) entity_string.append(side);
						entity_string.append("}\n");
					}

					// close submodel
					entity_string.append("}\n");

					submodel_num++;
				}
			}

			else
			{
				float t_origin[3] = {0.0f, 0.0f, 0.0f};

				if (!sscanf_s(origin.c_str(), "%f %f %f", &t_origin[0], &t_origin[1], &t_origin[2]))
				{
					game::Com_PrintMessage(0, utils::va("[!]: sscanf failed for entity %d", entity_num), 0);
				}

				if (utils::polylib::is_point_within_bounds(glm::to_vec3(t_origin), sbox->mins, sbox->maxs, 0.25f))
				{
					// start entity
					entity_string.append(utils::va("// entity %d\n", entity_num));
					entity_string.append("{\n");

					for (auto& property : entity)
					{
						entity_string.push_back('"');
						entity_string.append(property.first);
						entity_string.append("\" \"");
						entity_string.append(property.second);
						entity_string.append("\"\n");
					}

					// close entity
					entity_string.append("}\n");
					entity_num++;
				}
			}
		}

		return entity_string;
	}

	// only build worldspawn keys/values without opening/closing brackets
	std::string entities::build_worldspawn()
	{
		std::string entityString;

		for (auto& entity : this->entities_)
		{
			if (entity.find("classname") != entity.end())
			{
				if (entity["classname"] == "worldspawn"s)
				{
					for (auto& property : entity)
					{
						entityString.push_back('"');
						entityString.append(property.first);
						entityString.append("\" \"");
						entityString.append(property.second);
						entityString.append("\"\n");
					}

					break;
				}
			}
		}

		return entityString;
	}

	std::vector<std::string> entities::get_models()
	{
		std::vector<std::string> models;

		for (auto& entity : this->entities_)
		{
			if (entity.find("model") != entity.end())
			{
				std::string model = entity["model"];

				if (!model.empty() && model[0] != '*' && model[0] != '?') // Skip brushmodels
				{
					if (std::find(models.begin(), models.end(), model) == models.end())
					{
						models.push_back(model);
					}
				}
			}
		}

		return models;
	}

	std::vector<game::brushmodel_entity_s> entities::get_brushmodels()
	{
		// Index by the actual *N model number, independent of entity order.
		std::vector<game::brushmodel_entity_s> bmodels(game::cm->numSubModels);
		for (unsigned int i = 0; i < game::cm->numBrushes; ++i)
		{
			game::cm->brushes[i].isSubmodel = false;
			game::cm->brushes[i].cmSubmodelIndex = 0;
		}

		for (auto& entity : this->entities_)
		{
			const auto model = entity.find("model");
			if (model == entity.end() || model->second.empty() || model->second[0] != '*') continue;
			const auto number = model->second.substr(1);
			if (number.empty() || number.find_first_not_of("0123456789") != std::string::npos) continue;
			int index = 0;
			try { index = std::stoi(number); }
			catch (const std::exception&) { continue; }
			if (index <= 0 || index >= static_cast<int>(bmodels.size())) continue;
			const auto origin = entity.find("origin");
			if (origin != entity.end())
			{
				float parsed[3] = {};
				if (sscanf_s(origin->second.c_str(), "%f %f %f", &parsed[0], &parsed[1], &parsed[2]) == 3)
				{
					memcpy(bmodels[index].cm_submodel_origin, parsed, sizeof(parsed));
				}
			}
		}

		for (unsigned int index = 1; index < game::cm->numSubModels; ++index)
		{
			auto& model = bmodels[index];
			model.cm_submodel_index = static_cast<int>(index);
			model.cm_submodel = &game::cm->cmodels[index];
			model.cm_brush_indices = map_export::collect_brush_indices(game::cm->leafbrushNodes,
				game::cm->leafbrushNodesCount, model.cm_submodel->leaf.leafBrushNode, game::cm->numBrushes);
			for (const auto brush_index : model.cm_brush_indices)
			{
				auto& brush = game::cm->brushes[brush_index];
				brush.isSubmodel = true;
				brush.cmSubmodelIndex = static_cast<std::int16_t>(index);
			}
		}
		return bmodels;
	}

	void entities::delete_worldspawn()
	{
		for (auto i = this->entities_.begin(); i != this->entities_.end();)
		{
			if (i->find("classname") != i->end())
			{
				std::string classname = (*i)["classname"];
				if (utils::starts_with(classname, "worldspawn"))
				{
					i = this->entities_.erase(i);
					continue;
				}
			}

			++i;
		}
	}

	void entities::delete_triggers()
	{
		for (auto i = this->entities_.begin(); i != this->entities_.end();)
		{
			if (i->contains("classname"))
			{
				const std::string classname = (*i)["classname"];
				if (utils::starts_with(classname, "trigger_"))
				{
					i = this->entities_.erase(i);
					continue;
				}
			}

			++i;
		}
	}

	void entities::delete_weapons(bool keepTurrets)
	{
		for (auto i = this->entities_.begin(); i != this->entities_.end();)
		{
			if (   i->contains("weaponinfo") 
				|| i->contains("targetname") && (*i)["targetname"] == "oldschool_pickup"s)
			{
				if (   !keepTurrets 
					|| !i->contains("classname") 
					|| (*i)["classname"] != "misc_turret"s)
				{
					i = this->entities_.erase(i);
					continue;
				}
			}

			++i;
		}
	}

	void entities::parse(std::string buffer)
	{
		int parse_state= 0;

		std::string key;
		std::string value;

		std::unordered_map<std::string, std::string> entity;

		for (unsigned int i = 0; i < buffer.size(); ++i)
		{
			char character = buffer[i];
			if (character == '{')
			{
				entity.clear();
			}

			switch (character)
			{
				case '{':
				{
					entity.clear();
					break;
				}

				case '}':
				{
					this->entities_.push_back(entity);
					entity.clear();
					break;
				}

				case '"':
				{
					if (parse_state == PARSE_AWAIT_KEY)
					{
						key.clear();
						parse_state = PARSE_READ_KEY;
					}
					else if (parse_state == PARSE_READ_KEY)
					{
						parse_state = PARSE_AWAIT_VALUE;
					}
					else if (parse_state == PARSE_AWAIT_VALUE)
					{
						value.clear();
						parse_state = PARSE_READ_VALUE;
					}
					else if (parse_state == PARSE_READ_VALUE)
					{
						entity[utils::str_to_lower(key)] = value;
						parse_state = PARSE_AWAIT_KEY;
					}
					else
					{
						throw std::runtime_error("Parsing error!");
					}
					break;
				}

				default:
				{
					if (parse_state == PARSE_READ_KEY)
					{
						key.push_back(character);
					}
					else if (parse_state == PARSE_READ_VALUE)
					{
						value.push_back(character);
					}

					break;
				}
			}
		}
	}
}
