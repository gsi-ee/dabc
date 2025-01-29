if(APPLE)
set(_platform Darwin)
else()
set(_platform Linux)
endif()



dabc_link_library(
  DabcOlmd
  SOURCES   mbsapibase/f_his_hist.c
            mbsapibase/f_his_swpbas.c
            mbsapibase/f_his_toupper.c
            mbsapibase/f_mbs_status.c
            mbsapibase/f_stccomm.c
            mbsapibase/f_swaplw.c
            mbsapibase/f_ut_compress.c
            mbsapibase/f_ut_seg_show.c
            mbsapibase/f_ut_status.c
            mbsapibase/f_ut_time.c
                mbsapi/f_evcli.c
                mbsapi/f_evt.c
                mbsapi/fLmd.c
                mbsapi/f_radware.c
                mbsapi/f_ut_utime.c
                   src/Factory.cxx
                   src/OlmdInput.cxx
                   src/OlmdFile.cxx
  HEADERS   mbsapibase/err_mask_def.h
            mbsapibase/errnum_def.h
            mbsapibase/f_his_hist.h
            mbsapibase/f_his_swpbas.h
            mbsapibase/f_his_toupper.h
            mbsapibase/f_mbs_status.h
            mbsapibase/f_stccomm.h
            mbsapibase/f_swaplw.h
            mbsapibase/f_ut_compress.h
            mbsapibase/f_ut_seg_show.h
            mbsapibase/f_ut_status.h
            mbsapibase/f_ut_time.h
            mbsapibase/ml_def.h
            mbsapibase/mo_def.h
            mbsapibase/portnum_def.h
            mbsapibase/sbs_def.h
            mbsapibase/s_daqst.h
            mbsapibase/s_errstat.h
            mbsapibase/s_head.h
            mbsapibase/s_his_comm.h
            mbsapibase/s_his_head.h
            mbsapibase/s_pol_cond.h
            mbsapibase/s_set_ml.h
            mbsapibase/s_set_mo.h
            mbsapibase/s_setup.h
            mbsapibase/s_win_cond.h
            mbsapibase/sys_def.h
            mbsapibase/typedefs.h
            mbsapibase/typedefs_nt.h
                mbsapi/clnt_buf_def.h
                mbsapi/f_evcli.h
                mbsapi/f_evt.h
                mbsapi/fLmd.h
                mbsapi/f_radware.h
                mbsapi/f_ut_utime.h
                mbsapi/gps_sc_def.h
                mbsapi/s_bufhe.h
                mbsapi/s_bufhe_swap.h
                mbsapi/s_clntbuf.h
                mbsapi/s_clnt_filter.h
                mbsapi/s_clnt_filter_swap.h
                mbsapi/s_evhe.h
                mbsapi/s_evhe_swap.h
                mbsapi/s_filhe.h
                mbsapi/s_filhe_swap.h
                mbsapi/s_filter.h
                mbsapi/s_flt_descr.h
                mbsapi/s_flt_descr_swap.h
                mbsapi/sMbs.h
                mbsapi/s_opc1.h
                mbsapi/s_pat.h
                mbsapi/s_spe.h
                mbsapi/s_stdint.h
                mbsapi/s_ve10_1.h
                mbsapi/s_ve10_1_swap.h
                mbsapi/s_ves10_1.h
                mbsapi/s_ves10_1_swap.h
                 olmd/Factory.h
                 olmd/OlmdFile.h
                 olmd/OlmdInput.h
  INCDIR olmd
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  INCLUDES mbsapi mbsapibase
  DEFINITIONS ${_platform} _LARGEFILE64_SOURCE
  COPY_HEADERS
  NOWARN)

dabc_install_plugin_data(
  DabcOlmd
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/olmd)
