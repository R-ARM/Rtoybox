void *initProgList(int id) 
{ 
	log_debug("Initializing prog list\n"); 
	FILE *progs = fopen(HOME"/progs", "r");
	if (!progs)
	{
		log_err("Error reading progs!\n");
	}	
	struct Item *current;
	int ret;
	char tmp[64];
	char tmp2[64];
	char tmp3[64];
	char tmp4[64];

	progHead = (struct Item*)malloc(sizeof(struct Item)); 
	progHead->type = ITEM_TYPE_PARENT; 
	strcpy(progHead->name, "Prog");
	parents[id] = progHead;
	
	get_text_and_rect(renderer, "Prog", font, &progHead->texture, &progHead->rect, 255, 255, 255); 
	
	current = progHead; 
	
	while (1) 
	{

		current->next = (struct Item*)malloc(sizeof(struct Item)); 
		current->next->type = ITEM_TYPE_PROG; 
		current->next->prev = current;
		do
		{
#if 0
			ret = fscanf(progs, "%s %s %d %d", current->next->name, current->next->extraPath, &current->next->subtype, &current->next->type);
#else
			ret = fscanf(progs, "%s %s %s", current->next->mainPath, current->next->name, current->next->extraPath);
#endif
		} while (strncmp(current->next->mainPath, "#", 1) == 0);
#if 0
		if (current->next->type == ITEM_TYPE_LIST)
		{
			current->next->listHead = (struct listMember*)malloc(sizeof(struct listMember));

			struct listMember *firstElem;
			struct listMember *lastElem;
			struct listMember *currentList;
			//struct listMember *currentList = (struct listMember*)malloc(sizeof(struct listMember));
			currentList = current->next->listHead;
			firstElem = currentList;
			while (1)
			{
				fscanf(progs, "%s", tmp);
				if (strncmp(tmp, "}", 1) == 0)
				{
					//free(currentList->next);
					break;
				}


				if (strncmp(tmp, "col", 3) == 0)
				{
					fscanf(progs, "%s %d %d %d", currentList->name, &currentList->val1, &currentList->val2, &currentList->val3);
				}
				currentList->next = (struct listMember*)malloc(sizeof(struct listMember));
				currentList = currentList->next;
			}
			lastElem = currentList;
			lastElem->next = firstElem;
		}

#endif

		if (feof(progs) != 0)
		{ 
			free(current->next); 
			current->next = NULL; 
			break; 
		} 
		get_text_and_rect(renderer, current->next->name, font, &current->next->texture, &current->next->rect, 255, 255, 255); 
		current->next->next = NULL; 
		current = current->next; 
	} 
	
	log_debug("Done intializing prog list\n"); 
	fclose(progs); 
}