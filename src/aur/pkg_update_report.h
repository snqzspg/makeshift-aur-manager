#ifndef PKG_UPDATE_REPORT_H_INCLUDED
#define PKG_UPDATE_REPORT_H_INCLUDED

void perform_updates_summary_report(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict);

#endif // PKG_UPDATE_REPORT_H_INCLUDED