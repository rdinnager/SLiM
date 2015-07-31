//
//  slim_script_block.cpp
//  SLiM
//
//  Created by Ben Haller on 6/7/15.
//  Copyright (c) 2015 Messer Lab, http://messerlab.org/software/. All rights reserved.
//

#include "slim_eidos_block.h"
#include "eidos_interpreter.h"
#include "slim_global.h"

#include "errno.h"


using std::endl;
using std::string;


SLiMEidosScript::SLiMEidosScript(const string &p_script_string, int p_start_index) : EidosScript(p_script_string, p_start_index)
{
}

SLiMEidosScript::~SLiMEidosScript(void)
{
}

EidosASTNode *SLiMEidosScript::Parse_SLiMFile(void)
{
	EidosToken *virtual_token = new EidosToken(EidosTokenType::kTokenContextFile, gStr_empty_string, 0, 0);
	EidosASTNode *node = new EidosASTNode(virtual_token, true);
	
	while (current_token_type_ != EidosTokenType::kTokenEOF)
	{
		// We handle the grammar a bit differently than how it is printed in the railroad diagrams in the doc.
		// Parsing of the optional generation range is done in Parse_SLiMEidosBlock() since it ends up as children of that node.
		EidosASTNode *script_block = Parse_SLiMEidosBlock();
		
		node->AddChild(script_block);
	}
	
	Match(EidosTokenType::kTokenEOF, "SLiM file");
	
	return node;
}

EidosASTNode *SLiMEidosScript::Parse_SLiMEidosBlock(void)
{
	EidosToken *virtual_token = new EidosToken(EidosTokenType::kTokenContextEidosBlock, gStr_empty_string, 0, 0);
	EidosASTNode *slim_script_block_node = new EidosASTNode(virtual_token, true);
	
	// We handle the grammar a bit differently than how it is printed in the railroad diagrams in the doc.
	// We parse the slim_script_info section here, as part of the script block.
	if (current_token_type_ == EidosTokenType::kTokenString)
	{
		// a script identifier string is present; add it
		EidosASTNode *script_id_node = Parse_Constant();
		
		slim_script_block_node->AddChild(script_id_node);
	}
	
	if (current_token_type_ == EidosTokenType::kTokenNumber)
	{
		// A start generation is present; add it
		EidosASTNode *start_generation_node = Parse_Constant();
		
		slim_script_block_node->AddChild(start_generation_node);
		
		if (current_token_type_ == EidosTokenType::kTokenColon)
		{
			// An end generation is present; add it
			Match(EidosTokenType::kTokenColon, "SLiM script block");
			
			if (current_token_type_ == EidosTokenType::kTokenNumber)
			{
				EidosASTNode *end_generation_node = Parse_Constant();
				
				slim_script_block_node->AddChild(end_generation_node);
			}
			else
			{
				EIDOS_TERMINATION << "ERROR (Parse): unexpected token " << *current_token_ << " in Parse_SLiMEidosBlock" << eidos_terminate();
			}
		}
	}
	
	// Now we are to the point of parsing the actual slim_script_block
	if (current_token_type_ == EidosTokenType::kTokenIdentifier)
	{
		if (current_token_->token_string_.compare(gStr_initialize) == 0)
		{
			EidosASTNode *callback_info_node = new EidosASTNode(current_token_);
			
			Match(EidosTokenType::kTokenIdentifier, "SLiM initialize() callback");
			Match(EidosTokenType::kTokenLParen, "SLiM initialize() callback");
			Match(EidosTokenType::kTokenRParen, "SLiM initialize() callback");
			
			slim_script_block_node->AddChild(callback_info_node);
		}
		else if (current_token_->token_string_.compare(gStr_fitness) == 0)
		{
			EidosASTNode *callback_info_node = new EidosASTNode(current_token_);
			
			Match(EidosTokenType::kTokenIdentifier, "SLiM fitness() callback");
			Match(EidosTokenType::kTokenLParen, "SLiM fitness() callback");
			
			if (current_token_type_ == EidosTokenType::kTokenNumber)
			{
				// A (required) mutation type id is present; add it
				EidosASTNode *mutation_type_id_node = Parse_Constant();
				
				callback_info_node->AddChild(mutation_type_id_node);
			}
			else
			{
				EIDOS_TERMINATION << "ERROR (Parse): unexpected token " << *current_token_ << " in Parse_SLiMEidosBlock; a mutation type id is required in fitness() callback definitions" << eidos_terminate();
			}
			
			if (current_token_type_ == EidosTokenType::kTokenComma)
			{
				// A (optional) subpopulation id is present; add it
				Match(EidosTokenType::kTokenComma, "SLiM fitness() callback");
				
				if (current_token_type_ == EidosTokenType::kTokenNumber)
				{
					EidosASTNode *subpopulation_id_node = Parse_Constant();
					
					callback_info_node->AddChild(subpopulation_id_node);
				}
				else
				{
					EIDOS_TERMINATION << "ERROR (Parse): unexpected token " << *current_token_ << " in Parse_SLiMEidosBlock; a subpopulation id is expected after a comma in fitness() callback definitions" << eidos_terminate();
				}
			}
			
			Match(EidosTokenType::kTokenRParen, "SLiM fitness() callback");
			
			slim_script_block_node->AddChild(callback_info_node);
		}
		else if (current_token_->token_string_.compare(gStr_mateChoice) == 0)
		{
			EidosASTNode *callback_info_node = new EidosASTNode(current_token_);
			
			Match(EidosTokenType::kTokenIdentifier, "SLiM mateChoice() callback");
			Match(EidosTokenType::kTokenLParen, "SLiM mateChoice() callback");
			
			// A (optional) subpopulation id is present; add it
			if (current_token_type_ == EidosTokenType::kTokenNumber)
			{
				EidosASTNode *subpopulation_id_node = Parse_Constant();
				
				callback_info_node->AddChild(subpopulation_id_node);
			}
			
			Match(EidosTokenType::kTokenRParen, "SLiM mateChoice() callback");
			
			slim_script_block_node->AddChild(callback_info_node);
		}
		else if (current_token_->token_string_.compare(gStr_modifyChild) == 0)
		{
			EidosASTNode *callback_info_node = new EidosASTNode(current_token_);
			
			Match(EidosTokenType::kTokenIdentifier, "SLiM modifyChild() callback");
			Match(EidosTokenType::kTokenLParen, "SLiM modifyChild() callback");
			
			// A (optional) subpopulation id is present; add it
			if (current_token_type_ == EidosTokenType::kTokenNumber)
			{
				EidosASTNode *subpopulation_id_node = Parse_Constant();
				
				callback_info_node->AddChild(subpopulation_id_node);
			}
			
			Match(EidosTokenType::kTokenRParen, "SLiM modifyChild() callback");
			
			slim_script_block_node->AddChild(callback_info_node);
		}
	}
	
	// Regardless of what happened above, all Eidos blocks end with a compound statement, which is the last child of the node
	EidosASTNode *compound_statement_node = Parse_CompoundStatement();
	
	slim_script_block_node->AddChild(compound_statement_node);
	
	return slim_script_block_node;
}

void SLiMEidosScript::ParseSLiMFileToAST(void)
{
	// delete the existing AST
	delete parse_root_;
	parse_root_ = nullptr;
	
	// set up parse state
	parse_index_ = 0;
	current_token_ = token_stream_.at(parse_index_);		// should always have at least an EOF
	current_token_type_ = current_token_->token_type_;
	
	// parse a new AST from our start token
	EidosASTNode *tree = Parse_SLiMFile();
	
	tree->OptimizeTree();
	
	parse_root_ = tree;
	
	// if logging of the AST is requested, do that; always to cout, not to EIDOS_OUTSTREAM
	if (gEidosLogAST)
	{
		std::cout << "AST : \n";
		this->PrintAST(std::cout);
	}
}



SLiMEidosBlock::SLiMEidosBlock(EidosASTNode *p_root_node) : root_node_(p_root_node)
{
	const std::vector<EidosASTNode *> &block_children = root_node_->children_;
	int child_index = 0, n_children = (int)block_children.size();
	
	// eat a string, for the script id, if present
	block_id_ = -1;	// the default unless it is set below
	
	if ((child_index < n_children) && (block_children[child_index]->token_->token_type_ == EidosTokenType::kTokenString))
	{
		const string &id_string = block_children[child_index]->token_->token_string_;
		const char *id_cstr = id_string.c_str();
		int id_cstr_len = (int)strlen(id_cstr);
		
		if (*id_cstr != 's')
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the script block id must be a string that begins with \"s\"." << eidos_terminate();
		for (int str_index = 1; str_index < id_cstr_len; ++str_index)
			if ((id_cstr[str_index] < '0') || (id_cstr[str_index] > '9'))
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the script block id after the \"s\" prefix must be a simple integer." << eidos_terminate();
		
		if (id_cstr_len < 2)
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the script block id must have an integer identifier after the \"s\" prefix." << eidos_terminate();
		
		errno = 0;
		long long_block_id = strtol(id_cstr + 1, NULL, 10);	// +1 to omit the leading "s"
		
		if (errno)
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the script block id " << id_string << " was not parseable." << eidos_terminate();
		if (long_block_id > INT_MAX)
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the script block id " << id_string << " was out of range." << eidos_terminate();
		
		block_id_ = (int)long_block_id;
		child_index++;
	}
	
	// eat a number, for the start generation, if present
	if ((child_index < n_children) && (block_children[child_index]->token_->token_type_ == EidosTokenType::kTokenNumber))
	{
		start_generation_ = (int)EidosInterpreter::IntForNumberToken(block_children[child_index]->token_);
		end_generation_ = start_generation_;	// if a start is given, the default end is the same as the start
		child_index++;
		
		if ((start_generation_ < 1) || (start_generation_ > 1000000000L))
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the start generation " << start_generation_ << " was out of range." << eidos_terminate();
	}
	
	// eat a number, for the end generation, if present
	if ((child_index < n_children) && (block_children[child_index]->token_->token_type_ == EidosTokenType::kTokenNumber))
	{
		end_generation_ = (int)EidosInterpreter::IntForNumberToken(block_children[child_index]->token_);
		child_index++;
		
		if ((end_generation_ < 1) || (end_generation_ > 1000000000L))
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): the end generation " << end_generation_ << " was out of range." << eidos_terminate();
	}
	
	// eat the callback info node, if present
	if ((child_index < n_children) && (block_children[child_index]->token_->token_type_ != EidosTokenType::kTokenLBrace))
	{
		const EidosASTNode *callback_node = block_children[child_index];
		const EidosToken *callback_token = callback_node->token_;
		EidosTokenType callback_type = callback_token->token_type_;
		const std::string &callback_name = callback_token->token_string_;
		const std::vector<EidosASTNode *> &callback_children = callback_node->children_;
		int n_callback_children = (int)callback_children.size();
		
		if ((callback_type == EidosTokenType::kTokenIdentifier) && (callback_name.compare(gStr_initialize) == 0))
		{
			if (n_callback_children != 0)
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): initialize() callback needs 0 parameters" << eidos_terminate();
			
			if ((start_generation_ != -1) || (end_generation_ != INT_MAX))
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): a generation range cannot be specified for an initialize() callback" << eidos_terminate();
			
			start_generation_ = 0;
			end_generation_ = 0;
			type_ = SLiMEidosBlockType::SLiMEidosInitializeCallback;
		}
		else if ((callback_type == EidosTokenType::kTokenIdentifier) && (callback_name.compare(gStr_fitness) == 0))
		{
			if ((n_callback_children != 1) && (n_callback_children != 2))
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): fitness() callback needs 1 or 2 parameters" << eidos_terminate();
			
			mutation_type_id_ = (int)EidosInterpreter::IntForNumberToken(callback_children[0]->token_);
			
			if (n_callback_children == 2)
				subpopulation_id_ = (int)EidosInterpreter::IntForNumberToken(callback_children[1]->token_);
			
			type_ = SLiMEidosBlockType::SLiMEidosFitnessCallback;
		}
		else if ((callback_type == EidosTokenType::kTokenIdentifier) && (callback_name.compare(gStr_mateChoice) == 0))
		{
			if ((n_callback_children != 0) && (n_callback_children != 1))
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): mateChoice() callback needs 0 or 1 parameters" << eidos_terminate();
			
			if (n_callback_children == 1)
				subpopulation_id_ = (int)EidosInterpreter::IntForNumberToken(callback_children[0]->token_);
			
			type_ = SLiMEidosBlockType::SLiMEidosMateChoiceCallback;
		}
		else if ((callback_type == EidosTokenType::kTokenIdentifier) && (callback_name.compare(gStr_modifyChild) == 0))
		{
			if ((n_callback_children != 0) && (n_callback_children != 1))
				EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): modifyChild() callback needs 0 or 1 parameters" << eidos_terminate();
			
			if (n_callback_children == 1)
				subpopulation_id_ = (int)EidosInterpreter::IntForNumberToken(callback_children[0]->token_);
			
			type_ = SLiMEidosBlockType::SLiMEidosModifyChildCallback;
		}
		else
		{
			EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): unknown callback type" << eidos_terminate();
		}
		
		child_index++;
	}
	
	// eat the compound statement, which must be present
	if ((child_index < n_children) && (block_children[child_index]->token_->token_type_ == EidosTokenType::kTokenLBrace))
	{
		compound_statement_node_ = block_children[child_index];
		child_index++;
	}
	
	if (!compound_statement_node_)
		EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): no compound statement found for SLiMEidosBlock" << eidos_terminate();
	
	if (child_index != n_children)
		EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): unexpected node in SLiMEidosBlock" << eidos_terminate();
	
	ScanTree();
}

SLiMEidosBlock::SLiMEidosBlock(int p_id, const std::string &p_script_string, SLiMEidosBlockType p_type, int p_start, int p_end)
	: block_id_(p_id), type_(p_type), start_generation_(p_start), end_generation_(p_end)
{
	script_ = new EidosScript(p_script_string, 0);

	script_->Tokenize();
	script_->ParseInterpreterBlockToAST();
	
	root_node_ = script_->AST();
	
	if (root_node_->children_.size() != 1)
		EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): script blocks must be compound statements." << eidos_terminate();
	if (root_node_->children_[0]->token_->token_type_ != EidosTokenType::kTokenLBrace)
		EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::SLiMEidosBlock): script blocks must be compound statements." << eidos_terminate();
	
	compound_statement_node_ = root_node_->children_[0];
	
	ScanTree();
}

SLiMEidosBlock::~SLiMEidosBlock(void)
{
	delete script_;
	
	if (self_symbol_)
	{
		delete self_symbol_->second;
		delete self_symbol_;
	}
	if (script_block_symbol_)
	{
		delete script_block_symbol_->second;
		delete script_block_symbol_;
	}
	
	if (cached_value_block_id_)
		delete cached_value_block_id_;
}

void SLiMEidosBlock::_ScanNodeForIdentifiersUsed(const EidosASTNode *p_scan_node)
{
	// recurse down the tree; determine our children, then ourselves
	for (auto child : p_scan_node->children_)
		_ScanNodeForIdentifiersUsed(child);
	
	if (p_scan_node->token_->token_type_ == EidosTokenType::kTokenIdentifier)
	{
		const std::string &token_string = p_scan_node->token_->token_string_;
		
		if (token_string.compare(gStr_executeLambda) == 0)		contains_wildcard_ = true;
		if (token_string.compare(gStr_globals) == 0)			contains_wildcard_ = true;
		
		// ***** If a new flag is added here, it must also be added to the list in SLiMEidosBlock::ScanTree!
		
		if (token_string.compare(gStr_T) == 0)					eidos_contains_.contains_T_ = true;
		if (token_string.compare(gStr_F) == 0)					eidos_contains_.contains_F_ = true;
		if (token_string.compare(gStr_NULL) == 0)				eidos_contains_.contains_NULL_ = true;
		if (token_string.compare(gStr_PI) == 0)					eidos_contains_.contains_PI_ = true;
		if (token_string.compare(gStr_E) == 0)					eidos_contains_.contains_E_ = true;
		if (token_string.compare(gStr_INF) == 0)				eidos_contains_.contains_INF_ = true;
		if (token_string.compare(gStr_NAN) == 0)				eidos_contains_.contains_NAN_ = true;
		
		// look for instance identifiers like p1, g1, m1, s1; the heuristic here is very dumb, but errs on the safe side
		if (token_string.length() >= 2)
		{
			char char2 = token_string[1];
			
			if ((char2 >= '0') && (char2 <= '9'))
			{
				char char1 = token_string[0];
				
				if (char1 == 'p')								contains_pX_ = true;
				if (char1 == 'g')								contains_gX_ = true;
				if (char1 == 'm')								contains_mX_ = true;
				if (char1 == 's')								contains_sX_ = true;
			}
		}
		
		if (token_string.compare(gStr_sim) == 0)				contains_sim_ = true;
		if (token_string.compare(gStr_self) == 0)				contains_self_ = true;
		
		if (token_string.compare(gStr_mut) == 0)				contains_mut_ = true;
		if (token_string.compare(gStr_relFitness) == 0)			contains_relFitness_ = true;
		if (token_string.compare(gStr_genome1) == 0)			contains_genome1_ = true;
		if (token_string.compare(gStr_genome2) == 0)			contains_genome2_ = true;
		if (token_string.compare(gStr_subpop) == 0)				contains_subpop_ = true;
		if (token_string.compare(gStr_homozygous) == 0)			contains_homozygous_ = true;
		if (token_string.compare(gStr_sourceSubpop) == 0)		contains_sourceSubpop_ = true;
		if (token_string.compare(gStr_weights) == 0)			contains_weights_ = true;
		if (token_string.compare(gStr_childGenome1) == 0)		contains_childGenome1_ = true;
		if (token_string.compare(gStr_childGenome2) == 0)		contains_childGenome2_ = true;
		if (token_string.compare(gStr_childIsFemale) == 0)		contains_childIsFemale_ = true;
		if (token_string.compare(gStr_parent1Genome1) == 0)		contains_parent1Genome1_ = true;
		if (token_string.compare(gStr_parent1Genome2) == 0)		contains_parent1Genome2_ = true;
		if (token_string.compare(gStr_isCloning) == 0)			contains_isCloning_ = true;
		if (token_string.compare(gStr_isSelfing) == 0)			contains_isSelfing_ = true;
		if (token_string.compare(gStr_parent2Genome1) == 0)		contains_parent2Genome1_ = true;
		if (token_string.compare(gStr_parent2Genome2) == 0)		contains_parent2Genome2_ = true;
	}
}

void SLiMEidosBlock::ScanTree(void)
{
	_ScanNodeForIdentifiersUsed(compound_statement_node_);
	
	// If the script block contains a "wildcard" – an identifier that signifies that any other identifier could be accessed – then
	// we just set all of our "contains_" flags to T.  Any new flag that is added must be added here too!
	if (contains_wildcard_)
	{
		eidos_contains_.contains_T_ = true;
		eidos_contains_.contains_F_ = true;
		eidos_contains_.contains_NULL_ = true;
		eidos_contains_.contains_PI_ = true;
		eidos_contains_.contains_E_ = true;
		eidos_contains_.contains_INF_ = true;
		eidos_contains_.contains_NAN_ = true;
		contains_pX_ = true;
		contains_gX_ = true;
		contains_mX_ = true;
		contains_sX_ = true;
		contains_sim_ = true;
		contains_self_ = true;
		contains_mut_ = true;
		contains_relFitness_ = true;
		contains_genome1_ = true;
		contains_genome2_ = true;
		contains_subpop_ = true;
		contains_homozygous_ = true;
		contains_sourceSubpop_ = true;
		contains_weights_ = true;
		contains_childGenome1_ = true;
		contains_childGenome2_ = true;
		contains_childIsFemale_ = true;
		contains_parent1Genome1_ = true;
		contains_parent1Genome2_ = true;
		contains_isCloning_ = true;
		contains_isSelfing_ = true;
		contains_parent2Genome1_ = true;
		contains_parent2Genome2_ = true;
	}
}


//
// Eidos support
//

void SLiMEidosBlock::GenerateCachedSymbolTableEntry(void)
{
	// Note that this cache cannot be invalidated, because we are guaranteeing that this object will
	// live for at least as long as the symbol table it may be placed into!
	self_symbol_ = new EidosSymbolTableEntry(gStr_self, (new EidosValue_Object_singleton_const(this))->SetExternalPermanent());
}

void SLiMEidosBlock::GenerateCachedScriptBlockSymbolTableEntry(void)
{
	// Note that this cache cannot be invalidated, because we are guaranteeing that this object will
	// live for at least as long as the symbol table it may be placed into!
	if (block_id_ == -1)
		EIDOS_TERMINATION << "ERROR (SLiMEidosBlock::GenerateCachedSymbolTableEntry): internal error: cached symbol table entries for anonymous script blocks are not supported." << eidos_terminate();
	
	std::ostringstream script_stream;
	
	script_stream << "s" << block_id_;
	
	script_block_symbol_ = new EidosSymbolTableEntry(script_stream.str(), (new EidosValue_Object_singleton_const(this))->SetExternalPermanent());
}

const std::string *SLiMEidosBlock::ElementType(void) const
{
	return &gStr_SLiMEidosBlock;
}

void SLiMEidosBlock::Print(std::ostream &p_ostream) const
{
	p_ostream << *ElementType() << "<";
	
	if (start_generation_ > 0)
	{
		p_ostream << start_generation_;
		
		if (end_generation_ != start_generation_)
			p_ostream << ":" << end_generation_;
		
		p_ostream << " : ";
	}
	
	switch (type_)
	{
		case SLiMEidosBlockType::SLiMEidosEvent:				p_ostream << gStr_event; break;
		case SLiMEidosBlockType::SLiMEidosInitializeCallback:	p_ostream << gStr_initialize; break;
		case SLiMEidosBlockType::SLiMEidosFitnessCallback:		p_ostream << gStr_fitness; break;
		case SLiMEidosBlockType::SLiMEidosMateChoiceCallback:	p_ostream << gStr_mateChoice; break;
		case SLiMEidosBlockType::SLiMEidosModifyChildCallback:	p_ostream << gStr_modifyChild; break;
	}
	
	p_ostream << ">";
}

std::vector<std::string> SLiMEidosBlock::ReadOnlyMembers(void) const
{
	std::vector<std::string> constants = EidosObjectElement::ReadOnlyMembers();
	
	constants.push_back(gStr_id);			// block_id_
	constants.push_back(gStr_start);		// start_generation_
	constants.push_back(gStr_end);			// end_generation_
	constants.push_back(gStr_type);		// type_
	constants.push_back(gStr_source);		// source_
	
	return constants;
}

std::vector<std::string> SLiMEidosBlock::ReadWriteMembers(void) const
{
	std::vector<std::string> variables = EidosObjectElement::ReadWriteMembers();
	
	variables.push_back(gStr_active);		// active_
	variables.push_back(gStr_tag);					// tag_value_
	
	return variables;
}

bool SLiMEidosBlock::MemberIsReadOnly(EidosGlobalStringID p_member_id) const
{
	switch (p_member_id)
	{
			// constants
		case gID_id:
		case gID_start:
		case gID_end:
		case gID_type:
		case gID_source:
			return true;
			
			// variables
		case gID_active:
		case gID_tag:
			return false;
			
			// all others, including gID_none
		default:
			return EidosObjectElement::MemberIsReadOnly(p_member_id);
	}
}

EidosValue *SLiMEidosBlock::GetValueForMember(EidosGlobalStringID p_member_id)
{
	// All of our strings are in the global registry, so we can require a successful lookup
	switch (p_member_id)
	{
			// constants
		case gID_id:
		{
			// Note that this cache cannot be invalidated, because we are guaranteeing that this object will
			// live for at least as long as the symbol table it may be placed into!
			if (!cached_value_block_id_)
				cached_value_block_id_ = (new EidosValue_Int_singleton_const(block_id_))->SetExternalPermanent();
			return cached_value_block_id_;
		}
		case gID_start:
			return new EidosValue_Int_singleton_const(start_generation_);
		case gID_end:
			return new EidosValue_Int_singleton_const(end_generation_);
		case gID_type:
		{
			switch (type_)
			{
				case SLiMEidosBlockType::SLiMEidosEvent:				return new EidosValue_String(gStr_event);
				case SLiMEidosBlockType::SLiMEidosInitializeCallback:	return new EidosValue_String(gStr_initialize);
				case SLiMEidosBlockType::SLiMEidosFitnessCallback:		return new EidosValue_String(gStr_fitness);
				case SLiMEidosBlockType::SLiMEidosMateChoiceCallback:	return new EidosValue_String(gStr_mateChoice);
				case SLiMEidosBlockType::SLiMEidosModifyChildCallback:	return new EidosValue_String(gStr_modifyChild);
			}
		}
		case gID_source:
			return new EidosValue_String(compound_statement_node_->token_->token_string_);
			
			// variables
		case gID_active:
			return new EidosValue_Int_singleton_const(active_);
		case gID_tag:
			return new EidosValue_Int_singleton_const(tag_value_);
			
			// all others, including gID_none
		default:
			return EidosObjectElement::GetValueForMember(p_member_id);
	}
}

void SLiMEidosBlock::SetValueForMember(EidosGlobalStringID p_member_id, EidosValue *p_value)
{
	switch (p_member_id)
	{
		case gID_active:
		{
			TypeCheckValue(__func__, p_member_id, p_value, kValueMaskInt);
			
			active_ = p_value->IntAtIndex(0);
			
			return;
		}
	
		case gID_tag:
		{
			TypeCheckValue(__func__, p_member_id, p_value, kValueMaskInt);
			
			int64_t value = p_value->IntAtIndex(0);
			
			tag_value_ = value;
			return;
		}
			
			// all others, including gID_none
		default:
			return EidosObjectElement::SetValueForMember(p_member_id, p_value);
	}
}

std::vector<std::string> SLiMEidosBlock::Methods(void) const
{
	std::vector<std::string> methods = EidosObjectElement::Methods();
	
	return methods;
}

const EidosMethodSignature *SLiMEidosBlock::SignatureForMethod(EidosGlobalStringID p_method_id) const
{
	return EidosObjectElement::SignatureForMethod(p_method_id);
}

EidosValue *SLiMEidosBlock::ExecuteMethod(EidosGlobalStringID p_method_id, EidosValue *const *const p_arguments, int p_argument_count, EidosInterpreter &p_interpreter)
{
	return EidosObjectElement::ExecuteMethod(p_method_id, p_arguments, p_argument_count, p_interpreter);
}





























































