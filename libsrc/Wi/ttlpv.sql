--
--  $Id$
--
--  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
--  project.
--
--  Copyright (C) 1998-2022 OpenLink Software
--
--  This project is free software; you can redistribute it and/or modify it
--  under the terms of the GNU General Public License as published by the
--  Free Software Foundation; only version 2 of the License, dated June 1991.
--
--  This program is distributed in the hope that it will be useful, but
--  WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--  General Public License for more details.
--
--  You should have received a copy of the GNU General Public License along
--  with this program; if not, write to the Free Software Foundation, Inc.,
--  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
--
--


create procedure DB.DBA.L_O_LOOK (inout val_str varchar, inout dt_lang int, inout lng varchar, inout is_text int, inout id int)
{
  vectored;
  -- dbg_obj_princ ('L_O_LOOK (', val_str, dt_lang, lng, is_text, id, ')');
  declare fetched int;
  set triggers off;
  insert into DB.DBA.RDF_OBJ index RO_VAL option (fetch id by 'RDF_RO_ID' set fetched) (RO_VAL, RO_DT_AND_LANG, RO_ID) values (val_str, dt_lang, id);
  if (0 = fetched)
    {
      declare flags int;
      flags := case when is_text = 2 then 0 else is_text end;
      -- if (0 = flags)
      --   {
      --     dbg_obj_princ ('zero ro_flags in ttlpv,sql:37 ', val_str, lng);
      --     ;
      --   }
      insert into DB.DBA.RDF_OBJ index RDF_OBJ (RO_ID, RO_VAL, RO_FLAGS, RO_DT_AND_LANG, RO_LONG) values (id, val_str, flags, dt_lang, lng);
      if (1 = is_text)
        insert soft DB.DBA.VTLOG_DB_DBA_RDF_OBJ option (no cluster) (VTLOG_RO_ID, SNAPTIME, DMLTYPE) values (id, curdatetime (), 'I');
      if (2 = is_text)
	{
	  declare geo any;
	  if (lng is null)
	    geo := deserialize (val_str);
	  else
	    geo := deserialize (lng);
	  geo_insert ('DB.DBA.RDF_GEO', geo, id);
	  return;
	}
      declare pref varchar;
      if (lng is null)
      pref := subseq (val_str, 0, case when length (val_str) < 10 then length (val_str) else 10 end);
      else
        pref := subseq (lng, 0, 10);
      insert into DB.DBA.RO_START option (no cluster) (RS_START, RS_DT_AND_LANG, RS_RO_ID) values (pref, dt_lang, rdf_box (0, 257, 257, id, 0));
    }
  else if ((1 = is_text) and not __rdf_obj_ft_rule_check (null, null))
    {
      declare flags int;
      if (bit_and (is_text, 1) and exists (select 1 from DB.DBA.RDF_OBJ where RO_ID=id and not bit_and (RO_FLAGS, 1)))
        {
          update DB.DBA.RDF_OBJ set RO_FLAGS = bit_or (RO_FLAGS, 1) where RO_ID=id;
          insert soft DB.DBA.VTLOG_DB_DBA_RDF_OBJ option (no cluster) (VTLOG_RO_ID, SNAPTIME, DMLTYPE) values (id, curdatetime (), 'I');
        }
    }
}
;

create procedure DB.DBA.L_O_LOOK_NE (inout val_str varchar, inout dt_lang int, inout lng varchar, inout is_text int, inout id int)
{
  vectored;
  id := (select RO_ID from DB.DBA.RDF_OBJ table option (index RO_VAL) where RO_VAL = val_str and  RO_DT_AND_LANG = dt_lang);
  if (id is null)
    id := 0;
}
;

create procedure DB.DBA.RL_I2ID_NP (inout pref varchar, inout name varchar, inout id iri_id_8)
{
  vectored;
  declare pref_fetched, id_fetched, pref_id int;
  insert into DB.DBA.RDF_PREFIX index RDF_PREFIX option (fetch pref_id by 'RDF_PREF_SEQ' set pref_fetched) (RP_NAME, RP_ID) values (pref, pref_id);
  if (0 = pref_fetched)
    insert soft DB.DBA.RDF_PREFIX index DB_DBA_RDF_PREFIX_UNQC_RP_ID (RP_NAME, RP_ID) values (pref, pref_id);
  rdf_cache_id ('p', pref, pref_id);
  __rl_set_pref_id (name, pref_id);
  insert into DB.DBA.RDF_IRI index RDF_IRI option (fetch id by 'RDF_URL_IID_NAMED' set id_fetched) (RI_NAME, RI_ID) values (name, id);
  if (0 = id_fetched)
    insert into DB.DBA.RDF_IRI index DB_DBA_RDF_IRI_UNQC_RI_ID (RI_ID, RI_NAME) values (id, name);
}
;

create procedure DB.DBA.RL_I2ID (inout name varchar, inout id iri_id_8)
{
  vectored;
  declare id_fetched int;

  insert into DB.DBA.RDF_IRI index RDF_IRI option (fetch id by 'RDF_URL_IID_NAMED' set id_fetched) (RI_NAME, RI_ID) values (name, id);
  if (0 = id_fetched)
    insert into DB.DBA.RDF_IRI index DB_DBA_RDF_IRI_UNQC_RI_ID (RI_ID, RI_NAME) values (id, name);
  rdf_cache_id ('i', name, id);
}
;

create procedure DB.DBA.RL_I2ID_NE (inout name varchar, inout id iri_id_8)
{
  vectored;
  id := (select RI_ID from DB.DBA.RDF_IRI table option (index RDF_IRI) where RI_NAME = name);
  if (id is not null)
    rdf_cache_id ('i', name, id);
  else
    id := #i0;
}
;

create procedure RL_I2ID_NPE (inout pref varchar, inout name varchar, inout id iri_id_8)
{
  vectored;
  declare pref_fetched, id_fetched, pref_id int;
  pref_id := (select RP_ID from DB.DBA.RDF_PREFIX table option (index RDF_PREFIX) where RP_NAME = pref);
  if (pref_id is null)
    id := #i0;
  else
    {
      rdf_cache_id ('p', pref, pref_id);
      __rl_set_pref_id (name, pref_id);
      id := (select RI_ID from DB.DBA.RDF_IRI table option (index RDF_IRI) where RI_NAME = name);
      if (id is not null)
        rdf_cache_id ('i', name, id);
      else
        id := #i0;
    }
}
;


create procedure DB.DBA.TTLP_RL_TRIPLE (
  inout g_iid IRI_ID, inout s_uri varchar, inout p_uri varchar,
  inout o_uri varchar,
  inout app_env any )
{
  connection_set ('g_iid', g_iid);
  dpipe_input (app_env[1], s_uri, p_uri, o_uri, null);
  if (daq_buffered_bytes (app_env[1]) > 30000000 or dpipe_count (app_env[1]) >= sys_stat ('dc_max_batch_sz'))
    DB.DBA.RL_SEND (app_env, g_iid);
}
;

create procedure DB.DBA.RDF_RL_TYPE_ID (in iri varchar)
{
  declare id, old_mode int;
  id := (select RDT_TWOBYTE from DB.DBA.RDF_DATATYPE where RDT_QNAME = iri);
  if (id is not null)
    {
      rdf_cache_id ('t', iri, id);
      return id;
    }
  old_mode := log_enable (1, 1);
  declare n_dead int;
  n_dead := 1;
  declare exit handler for sqlstate '40001' {
    rollback work;
    n_dead := n_dead + 1;
    if (n_dead > 10)
      signal ('RDF..', 'Over 10 deadlocks getting datatype id. Retry the RDF load');
    goto again;
  };
 again:
  if (2 = old_mode)
    log_enable (0, 1);

  id := (select RDT_TWOBYTE from DB.DBA.RDF_DATATYPE where RDT_QNAME = iri);
  if (id is null)
    {
      declare t_iri_id any;
      t_iri_id := iri_to_id (iri, 1);
      id:= sequence_next ('RDF_DATATYPE_TWOBYTE', 1, 1);
      insert into DB.DBA.RDF_DATATYPE (RDT_TWOBYTE, RDT_IID, RDT_QNAME) values (id, t_iri_id, iri);
    }
  commit work; -- if load non transactional, this is still a sharp transaction boundary.
  log_enable (old_mode, 1);
  rdf_cache_id ('t', iri, id);
  return id;
}
;

create procedure DB.DBA.RDF_RL_LANG_ID (in ln varchar)
{
  declare id, old_mode int;
  declare daq  any;
  ln := lower (ln);
  id := (select RL_TWOBYTE from DB.DBA.RDF_LANGUAGE where RL_ID = ln);
  if (id is not null)
    {
      rdf_cache_id ('l', ln, id);
      return id;
    }
  old_mode := log_enable (1, 1);
  declare n_dead int;
  n_dead := 1;
  declare exit handler for sqlstate '40001' {
    rollback work;
    n_dead := n_dead + 1;
    if (n_dead > 10)
      signal ('RDF..', 'Over 10 deadlocks getting language id. Retry the RDF load');
    goto again;
  };
 again:
  if (2 = old_mode)
    log_enable (0, 1);
  id := (select RL_TWOBYTE from DB.DBA.RDF_LANGUAGE where RL_ID = ln);
  if (id is null)
    {
      id:= sequence_next ('RDF_LANGUAGE_TWOBYTE', 1, 1);
      insert into DB.DBA.RDF_LANGUAGE (RL_TWOBYTE, RL_ID) values (id, ln);
    }
  commit work; -- if load non transactional, this is still a sharp transaction boundary.
  log_enable (old_mode, 1);
  rdf_cache_id ('l', ln, id);
  return id;
}
;

create procedure DB.DBA.TTLP_RL_TRIPLE_L (
  inout g_iid IRI_ID, inout s_uri varchar, inout p_uri varchar,
  inout o_val any, inout o_type varchar, inout o_lang varchar,
  inout app_env any )
{
  declare is_text int;
  connection_set ('g_iid', g_iid);
  if (__rdf_obj_ft_rule_check (g_iid, p_uri))
    is_text := 1;
  if (o_type or o_lang)
    {
      declare o_val_2 any;
      declare lid, tid int;
      if (o_lang)
	{
	  lid := rdf_cache_id ('l', o_lang);
	    if (lid = 0)
	      lid := DB.DBA.RDF_RL_LANG_ID (o_lang);
	}
      else
        lid := 257;
      if (o_type)
	{
          declare parsed any;
          parsed := __xqf_str_parse_to_rdf_box (o_val, o_type, isstring (o_val));
          if (parsed is not null)
            {
              if (__tag of rdf_box = __tag (parsed))
                {
                  tid := rdf_cache_id ('t', o_type);
                  if (tid = 0)
                    tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
                  rdf_box_set_type (parsed, tid);
                }
              else if (__tag of XML = __tag (parsed))
                {
		  parsed := rdf_box (parsed, 300, 257, 0, 1);
		  tid := rdf_cache_id ('t', o_type);
		  if (tid = 0)
		    tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
		  rdf_box_set_type (parsed, tid);
		}
              -- if (not bit_and (is_text, 1))
              --   {
              --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:231', parsed);
              --     ;
              --   }
	      rdf_box_set_is_text (parsed, is_text);
              dpipe_input (app_env[1], s_uri, p_uri, null, parsed);
              goto do_flush; -- see below
            }
	  tid := rdf_cache_id ('t', o_type);
	  if (tid = 0)
	    tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
	}
      else
        tid := 257;
      o_val_2 := rdf_box (o_val, tid, lid, 0, 1);
      -- if (not bit_and (is_text, 1))
      --   {
      --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:241', o_val_2);
      --     ;
      --   }
      rdf_box_set_is_text (o_val_2, is_text);
      dpipe_input (app_env[1], s_uri, p_uri, null, o_val_2);
    }
  else
    {
      -- make first first non default type because if all is default it will make no box
      declare o_val_2 any;
      o_val_2 := rdf_box (o_val, 300, 257, 0, 1);
      -- if (not is_text)
      --   {
      --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:265', o_val_2);
      --     ;
      --   }
      if (is_text)
	rdf_box_set_is_text (o_val_2, 1);
      rdf_box_set_type (o_val_2, 257);
      dpipe_input (app_env[1], s_uri, p_uri, null, o_val_2);
    }
do_flush:
  if (daq_buffered_bytes (app_env[1]) > 30000000 or dpipe_count (app_env[1]) >= sys_stat ('dc_max_batch_sz'))
    DB.DBA.RL_SEND (app_env, g_iid);
}
;

create procedure DB.DBA.TTLP_RL_NEW_GRAPH (inout g varchar, inout g_iid IRI_ID, inout app_env any)
{
  -- dbg_obj_princ ('DB.DBA.TTLP_EV_NEW_GRAPH(', g, g_iid, app_env, ')');
  declare prev_g iri_id;
  prev_g := connection_get ('g_iid');
  if (dpipe_count (app_env[1]))
    DB.DBA.RL_SEND (app_env, prev_g);
}
;


create procedure DB.DBA.RL_LOCAL_DPIPE ()
{
  return dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO');  -- the pipe with CU_DB_ORDERED
}
;

create procedure DB.DBA.RL_LOCAL_DPIPE_GS ()
{
  return dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO', 'L_IRI_TO_ID');  -- the pipe with CU_DB_ORDERED
}
;

create procedure DB.DBA.RL_FLUSH (in dp any, in g_iid any)
{
  declare ro_id_dict any;
  if (1 or '1' = registry_get ('cl_rdf_text_index'))
    ro_id_dict := dict_new (2000);
  else
    ro_id_dict := null;
  connection_set ('g_dict', ro_id_dict);
  connection_set ('g_iid', g_iid);
  if (log_enable (null, 1) in (2,3))
    {
      set non_txn_insert = 1;
    }
  rl_dp_ids (dp, g_iid);
  connection_set ('g_dict', 0);
  connection_set ('g_iid', 0);
}
;

create procedure DB.DBA.RL_SEND (inout env any, in g_iid any)
{
  declare req, n_reqs int;
  commit work;
  -- dbg_obj_princ ('DB.DBA.RL_SEND (', env, g_iid, ')');
  n_reqs := env[2];
  env[2] := n_reqs + 1;
  req := aq_request (env[0], 'DB.DBA.RL_FLUSH', vector (env[1], g_iid));
  env[1] := DB.DBA.RL_LOCAL_DPIPE ();
  env[4 + mod (n_reqs, 5)] := req;
  if (n_reqs > 5)
    {
      commit work; -- it may happen the aq request before is executed on client thread
      aq_wait (env[0], env[4 + mod (n_reqs - 4, 5)], 1);
    }
}
;

create procedure DB.DBA.TTLP_RL_COMMIT (inout g varchar, inout app_env any)
{
  return;
}
;

create procedure DB.DBA.RL_SEND_GS (inout env any, in g_iid any)
{
  declare req, n_reqs int;
  if (bit_and (4, dpipe_rdf_load_mode (env[1])))
    return;
  -- dbg_obj_princ ('DB.DBA.RL_SEND_GS (', env, g_iid, ')');
  commit work;
  n_reqs := env[2];
  env[2] := n_reqs + 1;
  req := aq_request (env[0], 'DB.DBA.RL_FLUSH', vector (env[1], g_iid));
  env[1] := DB.DBA.RL_LOCAL_DPIPE_GS ();
  env[4 + mod (n_reqs, 5)] := req;
  if (n_reqs > 5)
    {
      commit work; -- it may happen the aq request before is executed on client thread
      aq_wait (env[0], env[4 + mod (n_reqs - 4, 5)], 1);
    }
}
;

create procedure DB.DBA.TTLP_RL_GS_TRIPLE (
  inout g_iid IRI_ID, inout s_uri varchar, inout p_uri varchar,
  inout o_uri varchar,
  inout app_env any )
{
  declare dp any;
  connection_set ('g_iid', g_iid);
 dp := app_env[1];
  dpipe_input (dp, s_uri, p_uri, o_uri, null, g_iid);
  if ((daq_buffered_bytes (dp) > 30000000 or dpipe_count (app_env[1]) >= sys_stat ('dc_max_batch_sz')) 
      and 0 = bit_and (4, dpipe_rdf_load_mode (dp)))
    DB.DBA.RL_SEND_GS (app_env, g_iid);
}
;

create procedure DB.DBA.TTLP_RL_GS_TRIPLE_L (
  inout g_iid IRI_ID, inout s_uri varchar, inout p_uri varchar,
  inout o_val any, inout o_type varchar, inout o_lang varchar,
  inout app_env any )
{
  declare dp any;
 dp := app_env[1];
  declare is_text int;
  if (isstring (g_iid))
    g_iid := __i2id (g_iid);
  connection_set ('g_iid', g_iid);
  if (__rdf_obj_ft_rule_check (g_iid, p_uri))
    is_text := 1;
  if (o_type or o_lang)
    {
      declare o_val_2 any;
      declare lid, tid int;
      if (o_lang)
	{
	  lid := rdf_cache_id ('l', o_lang);
	    if (lid = 0)
	      lid := DB.DBA.RDF_RL_LANG_ID (o_lang);
	}
      else
        lid := 257;
      if (o_type)
	{
          declare parsed any;
          parsed := __xqf_str_parse_to_rdf_box (o_val, o_type, isstring (o_val));
          if (parsed is not null)
            {
              if (__tag of rdf_box = __tag (parsed))
                {
                  tid := rdf_cache_id ('t', o_type);
                  if (tid = 0)
                    tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
                  rdf_box_set_type (parsed, tid);
                }
              else if (__tag of XML = __tag (parsed))
                {
		  parsed := rdf_box (parsed, 300, 257, 0, 1);
		  rdf_box_set_type (parsed, 257);
		}
              -- if (not bit_and (is_text, 1))
              --   {
              --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:401', parsed, is_text);
              --     ;
              --   }
	      rdf_box_set_is_text (parsed, is_text);
              dpipe_input (dp, s_uri, p_uri, null, parsed, g_iid);
              goto do_flush; -- see below
            }
	  tid := rdf_cache_id ('t', o_type);
	  if (tid = 0)
	    tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
	}
      else
        tid := 257;
      o_val_2 := rdf_box (o_val, tid, lid, 0, 1);
      -- if (not bit_and (is_text, 1))
      --   {
      --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:414', o_val_2, is_text);
      --     ;
      --   }
      rdf_box_set_is_text (o_val_2, is_text);
      dpipe_input (dp, s_uri, p_uri, null, o_val_2, g_iid);
    }
  else
    {
      -- make first first non default type because if all is default it will make no box
      declare o_val_2 any;
      o_val_2 := rdf_box (o_val, 300, 257, 0, 1);
      -- if (not is_text)
      --   {
      --     dbg_obj_princ ('zero set_is_text in ttlpv.sql:451', o_val_2);
      --     ;
      --   }
      if (is_text)
	rdf_box_set_is_text (o_val_2, 1);
      rdf_box_set_type (o_val_2, 257);
      dpipe_input (dp, s_uri, p_uri, null, o_val_2, g_iid);
    }
do_flush:
  if ((daq_buffered_bytes (dp) > 30000000 or dpipe_count (app_env[1]) >= sys_stat ('dc_max_batch_sz'))
      and 0 = bit_and (4, dpipe_rdf_load_mode (dp)))
    DB.DBA.RL_SEND_GS (app_env, g_iid);
}
;

create procedure DB.DBA.TTLP_RL_GS_NEW_GRAPH (inout g varchar, inout g_iid IRI_ID, inout app_env any)
{
  -- no op
  return;
}
;

create procedure DB.DBA.TTLP_EV_NULL_IID (inout uri varchar, inout g_iid IRI_ID, inout app_env any, inout res IRI_ID)
{
  res := uri;
}
;


--/1d1/tpch1000
--!!!
create procedure DB.DBA.TTLP_V_GS (in strg varchar, in base varchar, in graph varchar := null, in flags integer, in threads integer, in log_mode integer, in old_log_mode integer)
{
  declare ro_id_dict, app_env, g_iid any;
  -- dbg_obj_princ ('DB.DBA.TTLP_V_GS (...', base, graph, flags, threads, log_mode, old_log_mode);
  if (0 = sys_stat ('rdf_rpid64_mode'))
    signal ('22023', 'DB.DBA.TTLP_V_GS() cannot be used before upgrading the RDF_IRI table to 64-bit prefix IDs, use TTLP() instead');
  app_env := vector (async_queue (threads, 1), DB.DBA.RL_LOCAL_DPIPE_GS (), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  if (bit_and (flags, 2048))
    dpipe_set_rdf_load (app_env[1], 6); -- bit_or (RDF_DL_DEL_INS, RDF_LD_MULTIGRAPH)
  g_iid := case when length (graph) > 0 then iri_to_id (graph) else null end;
  rdf_load_turtle (strg, base, graph, flags,
    vector (
      'DB.DBA.TTLP_RL_GS_NEW_GRAPH',
      'DB.DBA.TTLP_EV_NEW_BLANK',
      'DB.DBA.TTLP_EV_NULL_IID',
      'DB.DBA.TTLP_RL_GS_TRIPLE',
      'DB.DBA.TTLP_RL_GS_TRIPLE_L',
      'DB.DBA.TTLP_RL_COMMIT',
      'DB.DBA.TTLP_EV_REPORT_DEFAULT' ),
    app_env);
  if (bit_and (4, dpipe_rdf_load_mode (app_env[1])))
    dpipe_exec_rdf_callback (app_env[1]);
  else
    {
      DB.DBA.RL_SEND_GS (app_env, g_iid);
      commit work;
      aq_wait_all (app_env[0]);
    }
  connection_set ('g_dict', null);
  if (old_log_mode is not null)
    log_enable (old_log_mode, 1);
}
;

create procedure DB.DBA.TTLP_V (in strg varchar, in base varchar, in graph varchar := null, in flags integer := 0, in threads int := 3, in transactional int := 0, in log_enable int := null)
{
  declare ro_id_dict, app_env, g_iid, old_log_mode any;
  if (1 <> sys_stat ('cl_run_local_only'))
    {
      DB.DBA.TTLP_CL (strg, 0, base, graph, flags);
      return;
    }
  if (is_atomic ())
    signal ('22023', 'DB.DBA.TTLP_V(), the vectorized Turtle loader, can not be used while server is in the atomic mode; consider using plain non-vectorised loader DB.DBA.TTLP()');
  if (0 = sys_stat ('rdf_rpid64_mode'))
    {
      DB.DBA.TTLP (strg, base, graph, flags);
      return;
    }
  old_log_mode := null;
  declare exit handler for sqlstate '37000' {
    if (app_env <> 0)
      {
    DB.DBA.RL_SEND (app_env, g_iid);
    commit work;
    aq_wait_all (app_env[0]);
      }
    connection_set ('g_dict', null);
    if (old_log_mode is not null)
    log_enable (old_log_mode, 1);
    signal (__SQL_STATE, '[Vectorized Turtle loader] ' || __SQL_MESSAGE);
  };
  if (transactional = 0)
    {
      if (log_enable = 0 or log_enable = 1)
        log_enable := 2 + log_enable;
    }
  else
    threads := 0;

  if (log_enable is not null)
      old_log_mode := log_enable (log_enable, 1);

  if (126 = __tag (strg))
    strg := cast (strg as varchar);

  if (bit_and (flags, 256+512))
    return DB.DBA.TTLP_V_GS (strg, base, graph, flags, threads, log_enable, old_log_mode);

 app_env := vector (async_queue (threads, 1), DB.DBA.RL_LOCAL_DPIPE (), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  g_iid := iri_to_id (graph);
  rdf_load_turtle (strg, base, graph, flags,
    vector (
      'DB.DBA.TTLP_RL_NEW_GRAPH',
      'DB.DBA.TTLP_EV_NEW_BLANK',
      'DB.DBA.TTLP_EV_GET_IID',
      'DB.DBA.TTLP_RL_TRIPLE',
      'DB.DBA.TTLP_RL_TRIPLE_L',
      'DB.DBA.TTLP_RL_COMMIT',
      'DB.DBA.TTLP_EV_REPORT_DEFAULT' ),
    app_env);
  DB.DBA.RL_SEND (app_env, g_iid);
  commit work;
  aq_wait_all (app_env[0]);
  connection_set ('g_dict', null);
  if (old_log_mode is not null)
    log_enable (old_log_mode, 1);
}
;

create procedure DB.DBA.RDF_LOAD_RDFXML_V (in strg varchar, in base varchar, in graph varchar := null, in threads int := 3, in transactional int := 0, in log_mode int := null, in parse_mode int := 0)
{
  declare ro_id_dict, app_env, g_iid, old_log_mode any;
  if (1 <> sys_stat ('cl_run_local_only'))
    return DB.DBA.RDF_LOAD_RDFXML_CL (strg, base, graph,0);
  if (0 = sys_stat ('rdf_rpid64_mode'))
    {
      DB.DBA.RDF_LOAD_RDFXML_IMPL (strg, base, graph, parse_mode, log_mode, transactional);
      return;
    }

  declare exit handler for sqlstate '37000' {
    DB.DBA.RL_SEND (app_env, g_iid);
    commit work;
    aq_wait_all (app_env[0]);
    connection_set ('g_dict', null);
    if (old_log_mode is not null)
    log_enable (old_log_mode, 1);
    signal (__SQL_STATE, '[Vectorized RDF/XML loader] ' || __SQL_MESSAGE);
  };
  old_log_mode := null;
  if (transactional = 0)
    {
      if (log_mode = 0 or log_mode = 1)
	log_mode := 2 + log_mode;
    }
  else
    threads := 0;
  if (log_mode is not null)
      old_log_mode := log_enable (log_mode, 1);
  if (126 = __tag (strg))
    {
      declare s any;
      s := string_output ();
      http (strg, s);
      strg := s;
    }
  app_env := vector (async_queue (threads, 1), DB.DBA.RL_LOCAL_DPIPE (), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  g_iid := iri_to_id (graph);
  rdf_load_rdfxml (strg, parse_mode, graph,
    vector (
      'DB.DBA.TTLP_RL_NEW_GRAPH',
      'DB.DBA.TTLP_EV_NEW_BLANK',
      'DB.DBA.TTLP_EV_GET_IID',
      'DB.DBA.TTLP_RL_TRIPLE',
      'DB.DBA.TTLP_RL_TRIPLE_L',
      'DB.DBA.TTLP_RL_COMMIT',
      'DB.DBA.TTLP_EV_REPORT_DEFAULT' ),
    app_env, base);
  DB.DBA.RL_SEND (app_env, g_iid);
  commit work;
  aq_wait_all (app_env[0]);
  connection_set ('g_dict', null);
  if (old_log_mode is not null)
    log_enable (old_log_mode, 1);
}
;

create procedure DB.DBA.ID_TO_IRI_VEC (in id iri_id)
{
  vectored;
  declare name, pref varchar;
  declare idn int;
  if (id is null)
    return id;
  if (not isiri_id (id))
    {
      if (__tag (id) = __tag of UNAME)
        return id;
      if (__tag (id) = __tag of varchar and bit_and (__box_flags (id), 1))
        return id;
    }
  if (is_plain_bnode_iri_id (id))
    return iri_id_to_blank_nodeid (id);
  name := rdf_cache_id_to_name ('i', id, 0);
  if (0 = name)
    {
      name := (select RI_NAME from DB.DBA.RDF_IRI where RI_ID = id);
      if (name is null)
        return sprintf_iri ('iri_id_%ld_with_no_name_entry', iri_id_num (id));
      else
        rdf_cache_id ('i', name, id);
    }
  pref := rdf_cache_id_to_name ('p', iri_name_id (name), 0);
  if (pref <> 0)
    return iri_from_pref_name (pref, name);
  pref := (select RP_NAME from DB.DBA.RDF_PREFIX where RP_ID = iri_name_id (name));
  if (pref is null)
    pref := 'no prefix';
  else
    rdf_cache_id ('p', pref, iri_name_id (name));
  return iri_from_pref_name (pref, name);
}
;


create procedure DB.DBA.ID_TO_IRI_VEC_NS (in id any array)
{
  vectored;
  declare name, pref varchar;
  declare idn int;
  if (not isiri_id (id))
    return (case (__tag (id))
      when __tag of UNAME then id
      when __tag of varchar then case when bit_and (__box_flags (id), 1) then id else NULL end
      else NULL end );
  if (is_plain_bnode_iri_id (id))
    return iri_id_to_blank_nodeid (id);
  name := rdf_cache_id_to_name ('i', idn, 0);
  if (0 = name)
    {
      name := (select RI_NAME from DB.DBA.RDF_IRI where RI_ID = id);
      if (name is null)
        return sprintf_iri ('iri_id_%ld_with_no_name_entry', iri_id_num (id));
      else
        rdf_cache_id ('i', name, id);
    }
  pref := rdf_cache_id_to_name ('p', iri_name_id (name), 0);
  if (pref <> 0)
    return iri_from_pref_name (pref, name);
  pref := (select RP_NAME from DB.DBA.RDF_PREFIX where RP_ID = iri_name_id (name));
  if (pref is null)
    pref := 'no prefix';
  else
    rdf_cache_id ('p', pref, iri_name_id (name));
  return iri_from_pref_name (pref, name);
}
;


create procedure DB.DBA.RDF_TRIPLES_BATCH_COMPLETE (inout triples any)
{
  declare tcount, tctr, vcount, vctr integer;
  declare inx, nt int;
  declare os, op, oo any array;
  nt := length (triples);
  for vectored (in t any array := triples, out os := s1, out op := p1, out oo := o1)
    {
      declare s1, p1, o1 any array;
      s1 := __ro2sq (t[0]);
      p1 := __ro2sq (t[1]);
      o1 := __ro2sq (t[2]);
    }
  for (inx := 0; inx < nt; inx := inx + 1)
    {
      declare obj any;
      triples[inx][0] := uriqa_dynamic_local_replace (os[inx]);
      triples[inx][1] := uriqa_dynamic_local_replace (op[inx]);
      obj := oo[inx];
      if (isstring (obj) and __box_flags (obj) = 1)
	triples[inx][2] := uriqa_dynamic_local_replace (obj);
      else	
	triples[inx][2] := obj;
    }
}
;

create procedure DB.DBA.RDF_VEC_INS_TRIPLES (in s any, in p any, in o any, in g any)
{
  vectored;
  not vectored {
    declare dp any array;
    declare is_local int;
    is_local := sys_stat ('cl_run_local_only');
    if (is_local)
      dp := dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO', 'L_IRI_TO_ID'); -- the pipe with CU_DB_ORDERED
    else
      {
        dp := dpipe (5, 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'MAKE_RO_1', 'IRI_TO_ID_1'); -- the pipe with bit_or (CU_DB_ORDERED, CU_ALLOW_REDO)
        dpipe_set_rdf_load (dp, 2); -- RDF_LD_MULTIGRAPH multiple graphs flag as G is given
      }
  }
  dpipe_input (dp, s, p, null, o, g);
  not vectored {
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
    if (is_local)
      rl_dp_ids (dp, 0);
    else
      {
	dpipe_next (dp, 0);
	dpipe_next (dp, 1);
      }
  }
}
;

--
-- o_flags
-- 0x0F: mask
-- 0x10: text will go in ft index
-- 0 - IRI
-- 1: complete box
-- 2: have lang in o_type
-- 3: parse literal dt is in o_type
-- 4: xmlliteral
-- 5: integer
-- 6: float
-- 7: double
-- 8: decimal
-- 15: IRI string
-- 0x100: S is BNode
-- 0x200: O is BNode

create procedure DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C (inout s any array, inout o any, in o_type any, in o_and_bnode_flags int, in is_local int)
{
  vectored;
  not vectored {
    declare xlat_dict any;
    xlat_dict := coalesce (connection_get ('RDF_INSERT_TRIPLE_C_BNODES', null), connection_get ('RDF_INSERT_TRIPLE_C_BNODES', null, 1));
    -- dbg_obj_princ ('DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C: xlat_dict=', xlat_dict);
  }
  declare o_flags, is_text int;
  declare lid, tid int;
  is_text := bit_and (o_and_bnode_flags, 16);
  o_flags := bit_and (o_and_bnode_flags, 15);
  -- dbg_obj_princ ('DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C: args are ', s, o, o_type, o_and_bnode_flags, is_local);
  if (xlat_dict is not null and (__tag(s) in (__tag of varchar, __tag of UNAME)) and (s like '[_]:%' or bit_and (o_and_bnode_flags, 0hex100)))
    {
    s := __bft ('nodeID://' || dict_get_or_set_sequence_next (xlat_dict, __bft (s,1), 'RDF_URL_IID_BLANK'), 1);
      -- dbg_obj_princ ('DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C: s is tweaked, now ', s);
    }
  if (o_flags in (0,15) and xlat_dict is not null and ((__tag(o) = __tag of UNAME) or (__tag(o) = __tag of varchar)) and (o like '[_]:%' or bit_and (o_and_bnode_flags, 0hex200)))
    {
      o := __bft ('nodeID://' || dict_get_or_set_sequence_next (xlat_dict, __bft(o,1), 'RDF_URL_IID_BLANK'), 1);
      return;
    }
  if (0 = o_flags)
    {
      o := __bft (o, 1);
      return;
    }
  else if (1 = o_flags)
    {
      o := rdf_box (o, 258, 257, 0, 1);
      rdf_box_set_type (o, 257);
      if (is_text and __tag of rdf_box = __tag (o))
	rdf_box_set_is_text (o, 1);
      return;
    }
  else if (2 = o_flags)
    {
      lid := rdf_cache_id ('l', lower (o_type));
      if (lid = 0)
	{
	  if (is_local)
            lid := DB.DBA.RDF_RL_LANG_ID (lower (o_type));
	  else
            lid := DB.DBA.RDF_LANG_ID (lower (o_type));
	}
      if (is_text and __tag of rdf_box = __tag (o))
	rdf_box_set_is_text (o, 1);
      o := rdf_box (o, 257, lid, 0, 1);
      return;
    }
  else if (3 = o_flags)
    {
      lid := 257;
      declare parsed any array;
      parsed := __xqf_str_parse_to_rdf_box (o, o_type, 1);
      if (parsed is not null)
        {
	  if (__tag of XML = __tag (parsed))
	    {
	      parsed := rdf_box (parsed, 300, 257, 0, 1);
	      rdf_box_set_type (parsed, 257);
	    }
          if (__tag of rdf_box = __tag (parsed))
            {
              if (257 = rdf_box_type (parsed))
                {
                  tid := rdf_cache_id ('t', o_type);
                  if (tid = 0)
                    {
                      if (is_local)
                        tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
                      else
                        tid := DB.DBA.RDF_TYPE_ID (o_type);
                    }
                  rdf_box_set_type (parsed, tid);
                }
            }
          o := parsed;
          return;
        }
      else
	{
	  tid := rdf_cache_id ('t', o_type);
	  if (tid = 0)
	    {
	      if (is_local)
                tid := DB.DBA.RDF_RL_TYPE_ID (o_type);
	      else
                tid := DB.DBA.RDF_TYPE_ID (o_type);
	    }
          o := rdf_box (o, tid, 257, 0, 1);
          if (is_text and __tag of rdf_box = __tag (o))
	    rdf_box_set_is_text (o, 1);
          return;
	}
    }
  else if (4 = o_flags)
    {
      o := rdf_box (xml_tree_doc (o), 300, 257, 0, 1);
      rdf_box_set_type (o, 257);
      if (is_text and __tag of rdf_box = __tag (o))
	rdf_box_set_is_text (o, 1);
      return;
    }
  else if (5 = o_flags)
    {
      o := cast (o as int);
      return;
    }
  else if (6 = o_flags)
    {
      o := cast (o as real);
      return;
    }
  else if (7 = o_flags)
    {
      o := cast (o as double precision);
      return;
    }
  else if (8 = o_flags)
    {
      o := cast (o as decimal);
      return;
    }
  else if (15 = o_flags)
    {
      o := __bft (o, 1);
      return;
    }
  else
    signal ('xxxxx', 'Bad rdf object flags va,value');
}
;

create procedure DB.DBA.RDF_INSERT_TRIPLE_C (in s any array, in p any array, in o any array, in o_type any array, in o_flags int, in g any array)
{
  vectored;
  not vectored {
    declare is_local int;
    is_local := sys_stat ('cl_run_local_only');
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
  }
  DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C (s, o, o_type, o_flags, is_local);
  not vectored {
    declare dp any array;
    if (is_local)
      dp := dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO', 'L_IRI_TO_ID'); -- the pipe with CU_DB_ORDERED
    else
      {
        dp := dpipe (5, 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'MAKE_RO_1', 'IRI_TO_ID_1'); -- the pipe with bit_or (CU_DB_ORDERED, CU_ALLOW_REDO)
        dpipe_set_rdf_load (dp, 2); -- RDF_LD_MULTIGRAPH
      }
  }
  --dbg_obj_princ ('After bnodes check, will call dpipe_input (dp, ', s, p, null, o, g, ')...');
  dpipe_input (dp, s, p, null, o, g);
  not vectored {
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
    if (is_local)
      rl_dp_ids (dp, 0);
    else
      {
	dpipe_next (dp, 0);
	dpipe_next (dp, 1);
      }
  }
}
;


create procedure DB.DBA.RDF_REPLACE_GRAPH_C (in s any array, in p any array, in o any array, in o_type any array, in o_flags int, in g any array)
{
  vectored;
  not vectored {
    declare is_local int;
    is_local := sys_stat ('cl_run_local_only');
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
  }
 o := DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C (s, o, o_type, o_flags, is_local);
  not vectored {
    declare dp any array;
    if (is_local)
      dp := dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO', 'L_IRI_TO_ID'); -- the pipe with CU_DB_ORDERED
    else
      {
        dp := dpipe (5, 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'MAKE_RO_1', 'IRI_TO_ID_1'); -- the pipe with bit_or (CU_DB_ORDERED, CU_ALLOW_REDO)
      }
    dpipe_set_rdf_load (dp, 6); -- bit_or (RDF_DL_DEL_INS, RDF_LD_MULTIGRAPH)
  }
  dpipe_input (dp, s, p, null, o, g);
  not vectored {
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
    if (is_local)
      {
	    dpipe_exec_rdf_callback (dp);
      }
    else
      {
	dpipe_next (dp, 0);
	dpipe_next (dp, 1);
      }
  }
}
;

create procedure DB.DBA.RDF_DELETE_TRIPLE_C (in s any array, in p any array, in o any array, in o_type any array, in o_flags int, in g any array)
{
  vectored;
  not vectored {
    declare is_local int;
    is_local := sys_stat ('cl_run_local_only');
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
  }
 o := DB.DBA.RDF_MAKE_S_O_FROM_PARTS_AND_FLAGS_C (s, o, o_type, o_flags, is_local);
  not vectored {
    declare dp any array;
    if (is_local)
    dp := dpipe (1, 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_IRI_TO_ID', 'L_MAKE_RO', 'L_IRI_TO_ID'); -- the pipe with CU_DB_ORDERED
    else
      {
        dp := dpipe (5, 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'IRI_TO_ID_1', 'MAKE_RO_1', 'IRI_TO_ID_1'); -- the pipe with bit_or (CU_DB_ORDERED, CU_ALLOW_REDO)
      }
    dpipe_set_rdf_load (dp, 3);
  }
  dpipe_input (dp, s, p, null, o, g);
  not vectored {
    if (log_enable (null, 1) in (2,3))
      set non_txn_insert = 1;
    if (is_local)
      rl_dp_ids (dp, 0);
    else
      {
	dpipe_next (dp, 0);
	dpipe_next (dp, 1);
      }
  }
}
;

create procedure DB.DBA.RDF_CLEAR_GRAPHS_C (in graphs any array)
{
  declare  kesg integer;
  foreach (any g_iri in graphs) do
    {
      kesg := __max (1, key_estimate('DB.DBA.RDF_QUAD', 'RDF_QUAD_GS', iri_to_id (g_iri, 0)));
      if (kesg <= 1000)
        {
          -- if graph is relatively small
          delete from DB.DBA.RDF_QUAD table option (index G) where G = iri_to_id (g_iri, 0);
        }
      else
        {
          delete from DB.DBA.RDF_QUAD where G = iri_to_id (g_iri, 0);
        }
      delete from DB.DBA.RDF_QUAD table option (index RDF_QUAD_GS, index_only) where G = iri_to_id (g_iri, 0) option (index_only, index RDF_QUAD_GS);

      -- Sponging
      delete from DB.DBA.SYS_HTTP_SPONGE where HS_LOCAL_IRI = g_iri;
      delete from DB.DBA.SYS_HTTP_SPONGE where HS_LOCAL_IRI like concat ('destMD5=', md5 (g_iri), '&graphMD5=%');
    }
}
;
